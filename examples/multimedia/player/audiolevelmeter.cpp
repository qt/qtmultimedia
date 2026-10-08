// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "audiolevelmeter.h"

#include <QApplication>
#include <QLabel>
#include <QPainter>
#include <QToolButton>
#include <QBoxLayout>

// Converts a float sample value to dB and clamps it between DB_MIN and DB_MAX
static float floatToDb(float f) {
    if (f <= 0)
        return DB_MIN;
    else
        return std::clamp(float(20.0) * std::log10(f), DB_MIN, DB_MAX);
}

AudioLevelMeter::AudioLevelMeter(QWidget *parent) : QWidget(parent)
{
    // Layout and background color
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    setMinimumWidth(WIDGET_WIDTH);
    QPalette currentPalette = palette();
    currentPalette.setColor(QPalette::Window, currentPalette.color(QPalette::Base));
    setPalette(currentPalette);
    setAutoFillBackground(true);
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(2);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Meter channels
    auto *meterChannelLayout = new QHBoxLayout();
    meterChannelLayout->setContentsMargins(2, 2, 2, 2);
    meterChannelLayout->setSpacing(2);
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        auto *channel = new MeterChannel(this);
        meterChannelLayout->addWidget(channel);
        m_channels[i] = channel;
    }
    mainLayout->addLayout(meterChannelLayout);

    // Peak label
    m_peakLabel = new QLabel("-", this);
    m_peakLabel->setAlignment(Qt::AlignCenter);
    QFont font = QApplication::font();
    font.setPointSize(10);
    m_peakLabel->setFont(font);
    mainLayout->addWidget(m_peakLabel);
    mainLayout->setStretch(0, 1);

    // On/off button
    m_onOffButton = new QToolButton(this);
    mainLayout->addWidget(m_onOffButton);
    m_onOffButton->setMaximumWidth(WIDGET_WIDTH);
    m_onOffButton->setText("On");
    m_onOffButton->setCheckable(true);
    m_onOffButton->setChecked(true);
    connect(m_onOffButton, &QToolButton::clicked, this, &AudioLevelMeter::toggleOnOff);

    // Timer triggering update of the audio level bars
    m_updateTimer.callOnTimeout(this, &AudioLevelMeter::updateBars);

    // Timer postponing deactivation of update timer to allow meters to fade to 0
    m_deactivationTimer.callOnTimeout(&m_updateTimer, &QTimer::stop);
    m_deactivationTimer.setSingleShot(true);

    // Timer resetting the peak label
    m_peakLabelHoldTimer.callOnTimeout(this, &AudioLevelMeter::resetPeakLabel);
    m_peakLabelHoldTimer.setSingleShot(true);

    // Buffer analyzer and worker thread that analyzes incoming buffers
    m_bufferAnalyzer = new BufferAnalyzer;
    m_bufferAnalyzer->moveToThread(&m_analyzerThread);
    connect(&m_analyzerThread, &QThread::finished, m_bufferAnalyzer, &QObject::deleteLater);
    connect(this, &AudioLevelMeter::newBuffer, m_bufferAnalyzer, &BufferAnalyzer::analyzeBuffer);
    connect(m_bufferAnalyzer, &BufferAnalyzer::valuesReady, this, &AudioLevelMeter::updateValues);
    m_analyzerThread.start();
}

// Analyzes an audio buffer and emits its peak and sumOfSquares values.
// Skips remaining frames if m_stopRrequested is set to true.
void BufferAnalyzer::analyzeBuffer(const QAudioBuffer &buffer, int maxChannelsToAnalyze)
{
    if (QThread::currentThread()->isInterruptionRequested())
        return; // Interrupted by ~AudioLevelMeter, skipping remaining buffers in signal queue

    m_stopRequested.store(false);

    int channelCount = buffer.format().channelCount();
    int channelsToAnalyze = std::min(channelCount, maxChannelsToAnalyze);

    BufferValues values(channelsToAnalyze);

    const unsigned char *bufferPtr = buffer.constData<unsigned char>();
    const unsigned char *endOfBuffer = bufferPtr + buffer.byteCount();
    int bytesPerSample = buffer.format().bytesPerSample();

    while (bufferPtr < endOfBuffer) {
        if (m_stopRequested.load()) {
            auto framesSkipped = (endOfBuffer - bufferPtr) / channelCount;
            qDebug() << "BufferAnalyzer::analyzeBuffer skipped"
                     << framesSkipped << "out of"
                     << buffer.frameCount() << "frames";
            // Emit incomplete values also when stop is requested to get some audio level readout
            // even if frames are being skipped for every buffer. Displayed levels will be
            // inaccurate.
            break;
        }

        for (int channelIndex = 0; channelIndex < channelsToAnalyze; ++channelIndex) {
            float sample = buffer.format().normalizedSampleValue(
                    bufferPtr + bytesPerSample * channelIndex);
            values.peaks[channelIndex] = std::max(values.peaks[channelIndex], std::abs(sample));
            values.squares[channelIndex] += sample * sample;
        }
        bufferPtr += bytesPerSample * channelCount;
    }

    emit valuesReady(values);
}

// Receives a buffer from QAudioBufferOutput and triggers BufferAnalyzer to analyze it
void AudioLevelMeter::onAudioBufferReceived(const QAudioBuffer &buffer)
{
    if (!m_isOn || !buffer.isValid() || !buffer.format().isValid())
        return;

    if (!m_isActive)
        activate();

    // Update internal values to match the current audio stream
    updateChannelCount(buffer.format().channelCount());
    m_frameCount = buffer.frameCount();
    m_bufferDurationMs = milliseconds(buffer.duration() / 1000);

    // Stop any ongoing analysis, skipping remaining frames
    m_bufferAnalyzer->requestStop();

    emit newBuffer(buffer, m_channelCount);
}

// Updates peak/RMS values and peak label
void AudioLevelMeter::updateValues(const BufferValues &values)
{
    if (!m_isActive)
        return; // Discard incoming values from BufferAnalyzer

    float bufferPeak = 0.0f;
    for (size_t i = 0; i < values.peaks.size(); ++i) {
        bufferPeak = std::max(bufferPeak, values.peaks[i]);
        m_channels[i]->updatePeak(values.peaks[i]);
        m_channels[i]->updateRms(values.squares[i], m_bufferDurationMs, m_frameCount);
    }
    updatePeakLabel(bufferPeak);
}

// Updates peak label and restarts m_peakLabelHoldTimer if peak >= m_highestPeak
void AudioLevelMeter::updatePeakLabel(float peak)
{
    if (peak < m_highestPeak)
        return;

    m_peakLabelHoldTimer.start(PEAK_LABEL_HOLD_TIME);

    if (qFuzzyCompare(peak, m_highestPeak))
        return;

    m_highestPeak = peak;
    float dB = floatToDb(m_highestPeak);
    m_peakLabel->setText(QString::number(dB, 'f', 1));
}

// Resets peak label. Called when m_labelHoldTimer timeouts
void AudioLevelMeter::resetPeakLabel()
{
    if (!m_isOn) {
        m_highestPeak = 0.0f;
        m_peakLabel->setText("");
        return;
    }

    m_highestPeak = 0.0f;
    m_peakLabel->setText(QString::number(DB_MIN, 'f', 1));
}

// Clears internal data used to calculate RMS values
void AudioLevelMeter::clearAllRmsData()
{
    for (MeterChannel *channel : m_channels)
        channel->clearRmsData();
}

// Starts the update timer that updates the meter bar
void AudioLevelMeter::activate()
{
    m_isActive = true;
    m_deactivationTimer.stop();
    m_updateTimer.start(UPDATE_INTERVAL);
}

// Start the deactiviation timer that eventually stops the update timer
void AudioLevelMeter::deactivate()
{
    m_isActive = false;
    clearAllRmsData();
    // Calculate the time it takes to decay fram max to min dB
    m_deactivationTimer.start(
            (DB_MAX - DB_MIN) / (DB_DECAY_PER_SECOND / 1000) + DECAY_EASE_IN_TIME.count());
}

// Decays internal peak and RMS values and triggers repainting of meter bars
void AudioLevelMeter::updateBars()
{
    for (int i = 0; i < m_channelCount; ++i) {
        MeterChannel *channel = m_channels.at(i);
        channel->decayPeak();
        channel->decayRms();
        channel->update(); // Trigger paint event
    }
}

// Toggles between on (activated) and off (deactivated) state.
void AudioLevelMeter::toggleOnOff()
{
    m_isOn = !m_isOn;
    if (!m_isOn)
        deactivate();
    else
        activate();
    m_onOffButton->setText(m_isOn ? "On" : "Off");
}

// Updates the number of visible MeterChannel widgets
void AudioLevelMeter::updateChannelCount(int channelCount)
{
    if (channelCount == m_channelCount
        || (channelCount > MAX_CHANNELS && MAX_CHANNELS == m_channelCount)) {
        return;
    }

    m_channelCount = std::min(channelCount, MAX_CHANNELS);

    for (int i = 0; i < MAX_CHANNELS; ++i)
        m_channels.at(i)->setVisible(i < m_channelCount);
}

MeterChannel::MeterChannel(QWidget *parent)
    : QWidget(parent)
{
}

// Normalizes a dB value for visualization
float MeterChannel::normalize(float dB)
{
    return (dB - DB_MIN) / (DB_MAX - DB_MIN);
}

// Clears the data used to calculate RMS values
void MeterChannel::clearRmsData()
{
    m_sumOfSquares = 0.0f;
    m_sumOfSquaresQueue = std::queue<float>();
}

// Decays m_peak value by DB_DECAY_PER_UPDATE with ease-in animation based on DECAY_EASE_IN_TIME
void MeterChannel::decayPeak()
{
    float peak = m_peak;
    if (qFuzzyCompare(peak, DB_MIN))
        return;

    float cubicEaseInFactor = m_peakDecayRate * m_peakDecayRate * m_peakDecayRate;
    m_peak = std::max(DB_MIN, peak - DB_DECAY_PER_UPDATE * cubicEaseInFactor);

    if (m_peakDecayRate < 1) {
        m_peakDecayRate += static_cast<float>(UPDATE_INTERVAL.count())
                / DECAY_EASE_IN_TIME.count();
        if (m_peakDecayRate > 1.0f)
            m_peakDecayRate = 1.0f;
    }
}

// Decays m_rms value by DB_DECAY_PER_UPDATE with ease-in animation based on DECAY_EASE_IN_TIME
void MeterChannel::decayRms()
{
    float rms = m_rms;
    if (qFuzzyCompare(rms, DB_MIN))
        return;

    float cubicEaseInFactor = m_rmsDecayRate * m_rmsDecayRate * m_rmsDecayRate;
    m_rms = std::max(DB_MIN, rms - DB_DECAY_PER_UPDATE * cubicEaseInFactor);

    if (m_rmsDecayRate < 1) {
        m_rmsDecayRate += static_cast<float>(UPDATE_INTERVAL.count())
                / DECAY_EASE_IN_TIME.count();
        if (m_rmsDecayRate > 1.0f)
            m_rmsDecayRate = 1.0f;
    }
}

// Updates m_peak and resets m_peakDecayRate if sampleValue > m_peak
void MeterChannel::updatePeak(float sampleValue)
{
    float dB = floatToDb(sampleValue);
    if (dB > m_peak) {
        m_peakDecayRate = 0;
        m_peak = dB;
    }
}

// Calculates current RMS. Resets m_rmsDecayRate and updates m_rms if current RMS > m_rms.
void MeterChannel::updateRms(float sumOfSquaresForOneBuffer, milliseconds duration, int frameCount)
{
    // Add the new sumOfSquares to the queue and update the total
    m_sumOfSquaresQueue.push(sumOfSquaresForOneBuffer);
    m_sumOfSquares += sumOfSquaresForOneBuffer;

    // Remove the oldest sumOfSquares to stay within the RMS window
    if (int64_t(m_sumOfSquaresQueue.size() * duration.count()) > int64_t(RMS_WINDOW.count())) {
        m_sumOfSquares -= m_sumOfSquaresQueue.front();
        m_sumOfSquaresQueue.pop();
    }

    // Fix negative values caused by floating point precision errors
    if (m_sumOfSquares < 0)
        m_sumOfSquares = 0;

    // Calculate the new RMS value
    if (m_sumOfSquares > 0 && m_sumOfSquaresQueue.size() > 0) {
        float newRms = std::sqrt(m_sumOfSquares / (frameCount * m_sumOfSquaresQueue.size()));
        float dB = floatToDb(newRms);
        if (dB > m_rms) {
            m_rmsDecayRate = 0;
            m_rms = dB;
        }
    }
}

// Paints the level bar of the meter channel based on the decayed peak and rms values.
void MeterChannel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    if (qFuzzyCompare(m_peak, DB_MIN) && qFuzzyCompare(m_rms, DB_MIN))
        return; // Nothing to paint

    float peakLevel = normalize(m_peak);
    float rmsLevel = normalize(m_rms);

    QPainter painter(this);
    auto rect = QRectF(
            0,
            height(),
            width(),
            -peakLevel * height());

    painter.fillRect(rect, m_peakBrush); // Paint the peak level
    rect.setHeight(-rmsLevel * height());
    painter.fillRect(rect, m_rmsBrush); // Paint the RMS level
}

#include "moc_audiolevelmeter.cpp"
