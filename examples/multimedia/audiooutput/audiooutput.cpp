// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "audiooutput.h"

#include <QVBoxLayout>

namespace {

// IIR filter with poles on the unit circle to generate a sine wave
// https://ccrma.stanford.edu/~jos/pasp/Digital_Sinusoid_Generators.html
struct SineOscillator
{
    SineOscillator(float frequency, float sampleRate)
    {
        float omega = 2.0 * M_PI * frequency / sampleRate;
        b1 = 2.0f * qCos(omega);
        float initialPhase = 0.0f;
        y1 = qSin(initialPhase - omega);
        y2 = qSin(initialPhase - 2.0f * omega);
    }

    float nextSample()
    {
        double y0 = b1 * y1 - y2;
        y2 = y1;
        y1 = y0;
        return float(std::clamp(y0, -1.0, 1.0));
    }

private:
    double b1{};
    double y1{};
    double y2{};
};

uint8_t asUint8(float normalizedFloat)
{
    return quint8((1.0 + normalizedFloat) / 2 * 255);
}

uint16_t asInt16(float normalizedFloat)
{
    return qint16(normalizedFloat * 32767);
}

uint32_t asInt32(float normalizedFloat)
{
    return qint32(normalizedFloat * double(std::numeric_limits<qint32>::max()));
}

QSpan<char> writeFromSampleValue(QSpan<char> buffer, float value, QAudioFormat::SampleFormat format)
{
    switch (format) {
    case QAudioFormat::UInt8: {
        *reinterpret_cast<quint8 *>(buffer.data()) = asUint8(value);
        return buffer.subspan(sizeof(quint8));
    }
    case QAudioFormat::Int16: {
        *reinterpret_cast<qint16 *>(buffer.data()) = asInt16(value);
        return buffer.subspan(sizeof(qint16));
    }
    case QAudioFormat::Int32: {
        *reinterpret_cast<qint32 *>(buffer.data()) = asInt32(value);
        return buffer.subspan(sizeof(qint32));
    }
    case QAudioFormat::Float: {
        *reinterpret_cast<float *>(buffer.data()) = value;
        return buffer.subspan(sizeof(float));
    }
    default:
        Q_UNREACHABLE_RETURN(buffer);
    }
}

} // namespace

Generator::Generator(const QAudioFormat &format, qint64 durationUs, int frequency)
{
    if (format.isValid())
        generateData(format, durationUs, frequency);
}

void Generator::start()
{
    open(QIODevice::ReadOnly);
}

void Generator::stop()
{
    m_pos = 0;
    close();
}

void Generator::generateData(const QAudioFormat &format, qint64 durationUs, int frequency)
{
    qint64 bytes = format.bytesForDuration(durationUs);
    m_buffer.resize(bytes);
    QSpan<char> buffer(m_buffer.data(), m_buffer.size());
    auto osc = SineOscillator(frequency, format.sampleRate());

    while (!buffer.empty()) {
        const float sampleValue = osc.nextSample(); // Produces value (-1..1)
        for (int i = 0; i < format.channelCount(); ++i)
            buffer = writeFromSampleValue(buffer, sampleValue, format.sampleFormat());
    }
}

qint64 Generator::readData(char *data, qint64 len)
{
    qint64 total = 0;
    if (!m_buffer.isEmpty()) {
        while (len - total > 0) {
            const qint64 chunk = qMin((m_buffer.size() - m_pos), len - total);
            memcpy(data + total, m_buffer.constData() + m_pos, chunk);
            m_pos = (m_pos + chunk) % m_buffer.size();
            total += chunk;
        }
    }
    return total;
}

qint64 Generator::writeData([[maybe_unused]] const char *data, [[maybe_unused]] qint64 len)
{
    return 0;
}

qint64 Generator::bytesAvailable() const
{
    return m_buffer.size() + QIODevice::bytesAvailable();
}

using namespace Qt::Literals::StringLiterals;
static QString sampleFormatToString(QAudioFormat::SampleFormat f)
{
    switch (f) {
    case QAudioFormat::UInt8: return u"UInt8"_s;
    case QAudioFormat::Int16: return u"Int16"_s;
    case QAudioFormat::Int32: return u"Int32"_s;
    case QAudioFormat::Float: return u"Float"_s;
    default:                  return u"Unknown"_s;
    }
}

static constexpr std::array allSupportedSampleRates{
    8'000, 11'025, 12'000, 16'000, 22'050, 24'000, 32'000, 44'100,
    48'000, 64'000, 88'200, 96'000, 128'000, 176'400, 192'000,
};

template <typename T>
static void setCurrentValue(QComboBox *box, const T &value)
{
    int idx = box->findData(QVariant::fromValue(value));
    if (idx >= 0)
        box->setCurrentIndex(idx);
}

static void syncFormatGui(QComboBox *m_formatBox, QComboBox *m_channelsBox, QComboBox *m_rateBox,
                          const QAudioFormat &format)
{
    setCurrentValue(m_formatBox, format.sampleFormat());
    setCurrentValue(m_rateBox, format.sampleRate());
    setCurrentValue(m_channelsBox, format.channelCount());
}

AudioTest::AudioTest() : m_devices(new QMediaDevices(this)), m_pushTimer(new QTimer(this))
{
    initializeWindow();

    // deviceChanged will kickstart the QAudioSink
    deviceChanged(m_deviceBox->currentIndex());
}

AudioTest::~AudioTest()
{
    m_pushTimer->stop();
    cleanupAudioSink();
}

void AudioTest::initializeWindow()
{
    QWidget *window = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;

    m_deviceBox = new QComboBox(this);
    QAudioDevice defaultDevice = QMediaDevices::defaultAudioOutput();
    for (auto &deviceInfo : QMediaDevices::audioOutputs())
        m_deviceBox->addItem(deviceInfo.description(), QVariant::fromValue(deviceInfo));
    auto defaultDeviceIndex = m_deviceBox->findData(QVariant::fromValue(defaultDevice));
    m_deviceBox->setCurrentIndex(defaultDeviceIndex);

    connect(m_deviceBox, &QComboBox::currentIndexChanged, this, &AudioTest::deviceChanged);
    connect(m_devices, &QMediaDevices::audioOutputsChanged, this, &AudioTest::updateAudioDevices);
    layout->addWidget(m_deviceBox);

    m_modeBox = new QComboBox(this);
    m_modeBox->addItem(tr("Pull Mode"));
    m_modeBox->addItem(tr("Push Mode"));
    m_modeBox->addItem(tr("Callback Mode"));
    connect(m_modeBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_mode = AudioTestMode{ index };
        restartAudioStream();
    });
    layout->addWidget(m_modeBox);
    m_modeBox->setCurrentIndex(qToUnderlying(m_mode));

    m_suspendResumeButton = new QPushButton(this);
    connect(m_suspendResumeButton, &QPushButton::clicked, this, &AudioTest::toggleSuspendResume);
    layout->addWidget(m_suspendResumeButton);

    QHBoxLayout *volumeBox = new QHBoxLayout;
    m_volumeLabel = new QLabel;
    m_volumeLabel->setText(tr("Volume:"));
    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setMinimum(0);
    m_volumeSlider->setMaximum(100);
    m_volumeSlider->setSingleStep(10);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &AudioTest::volumeChanged);
    volumeBox->addWidget(m_volumeLabel);
    volumeBox->addWidget(m_volumeSlider);
    layout->addLayout(volumeBox);

    // Sample Format selector
    QHBoxLayout *formatBox = new QHBoxLayout;
    QLabel *formatLabel = new QLabel;
    formatLabel->setText(tr("Sample Format:"));
    m_formatBox = new QComboBox(this);

    //Sample rate button
    QLabel *rateLabel = new QLabel;
    rateLabel->setText(tr("Sample Rate:"));
    m_rateBox = new QComboBox(this);

    // setting channel count
    QLabel *chLabel = new QLabel;
    chLabel->setText(tr("Channels:"));
    m_channelsBox = new QComboBox(this);

    for (auto *box : { m_channelsBox, m_rateBox, m_formatBox }) {
        connect(box, &QComboBox::activated, this, [this, box]() {
            formatChanged(box);
        });
    }

    // add all to the same row
    formatBox->addWidget(formatLabel);
    formatBox->addWidget(m_formatBox);
    formatBox->addSpacing(12);
    formatBox->addWidget(rateLabel);
    formatBox->addWidget(m_rateBox);
    formatBox->addSpacing(12);
    formatBox->addWidget(chLabel);
    formatBox->addWidget(m_channelsBox);

    layout->addLayout(formatBox);

    window->setLayout(layout);

    setCentralWidget(window);
    window->show();
}

void AudioTest::startAudioSink(const QAudioDevice &device, const QAudioFormat &format)
{
    if (m_audioSink)
        cleanupAudioSink();

    m_audioSink = std::make_unique<QAudioSink>(device, format);
    m_audioSink->setVolume(0.25f); // roughly -12dB

    m_currentDevice = device;

    syncFormatGui(m_formatBox, m_channelsBox, m_rateBox, m_audioSink->format());

    // handle startup/runtime errors and success negotiation
    connect(m_audioSink.get(), &QAudioSink::stateChanged, this, [this, device](QAudio::State s) {
        switch (s) {
        case QAudio::ActiveState:
            m_suspendResumeButton->setText(tr("Suspend playback"));
            return;

        case QAudio::SuspendedState:
            m_suspendResumeButton->setText(tr("Resume playback"));
            return;

        default:
            break;
        }

        const auto err = m_audioSink->error();

        // startup failure (format rejected or device unavailable)
        if (err == QAudio::OpenError && s == QAudio::StoppedState) {
            QMessageBox::warning(this, tr("Audio start failed"),
                                 tr("Device rejected the format or is unavailable."));
            return;
        }

        // runtime I/O or fatal device error (disconnects, etc.)
        if (err == QAudio::IOError || err == QAudio::FatalError) {
            if (m_currentDevice == device) {
                m_currentDevice = {};
                m_deviceBox->setCurrentIndex(-1);
            }
            QMessageBox::warning(this, tr("Audio error"), tr("Audio device error."));
            return;
        }
    });

    // set initial volume and kick the stream
    qreal initialVolume = QAudio::convertVolume(m_audioSink->volume(),
                                                QAudio::LinearVolumeScale,
                                                QAudio::LogarithmicVolumeScale);
    m_volumeSlider->setValue(qRound(initialVolume * 100));

    restartAudioStream();
}

void AudioTest::deviceChanged(int index)
{
    QAudioDevice dev = m_deviceBox->itemData(index).value<QAudioDevice>();

    // formats
    m_formatBox->clear();
    m_channelsBox->clear();
    m_rateBox->clear();

    if (!dev.isNull()) {
        const auto formats = dev.supportedSampleFormats();
        for (const QAudioFormat::SampleFormat sf : formats)
            m_formatBox->addItem(sampleFormatToString(sf), QVariant::fromValue(sf));

        // channels
        for (int ch = dev.minimumChannelCount(); ch <= dev.maximumChannelCount(); ++ch)
            m_channelsBox->addItem(QString::number(ch), ch);

        // populate from the hardcoded list in this cpp file
        for (int rate : allSupportedSampleRates) {
            if (rate < dev.minimumSampleRate() || rate > dev.maximumSampleRate())
                continue;
            m_rateBox->addItem(QString::number(rate), rate);
        }
    }

    if (dev != m_currentDevice) {
        cleanupAudioSink();
        if (!dev.isNull()) {
            QAudioFormat format = dev.preferredFormat();
            startAudioSink(dev, format);
        }
    }
}

void AudioTest::volumeChanged(int value)
{
    qreal linearVolume = QAudio::convertVolume(value / qreal(100),
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);

    m_audioSink->setVolume(linearVolume);
}

void AudioTest::formatChanged(QComboBox *box)
{
    QAudioDevice device = m_deviceBox->currentData().value<QAudioDevice>();
    QAudioFormat newFormat = m_audioSink->format();

    if (box == m_formatBox) {
        newFormat.setSampleFormat(
            static_cast<QAudioFormat::SampleFormat>(box->currentData().toInt()));
    } else if (box == m_rateBox) {
        newFormat.setSampleRate(box->currentData().toInt());
    } else if (box == m_channelsBox) {
        newFormat.setChannelCount(box->currentData().toInt());
    }

    startAudioSink(device, newFormat);
}

void AudioTest::updateAudioDevices()
{
    QSignalBlocker blockUpdates(m_deviceBox);

    m_deviceBox->clear();

    const QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
    for (const QAudioDevice &deviceInfo : devices)
        m_deviceBox->addItem(deviceInfo.description(), QVariant::fromValue(deviceInfo));
    const int currentDeviceIndex = m_deviceBox->findData(QVariant::fromValue(m_currentDevice));
    if (currentDeviceIndex != -1) {
        // select previous device
        m_deviceBox->setCurrentIndex(currentDeviceIndex);
    } else {
        blockUpdates.unblock();
        // select default device
        QAudioDevice defaultDevice = QMediaDevices::defaultAudioOutput();
        const int defaultDeviceIndex = m_deviceBox->findData(QVariant::fromValue(defaultDevice));
        const int currentIndex = m_deviceBox->currentIndex();
        m_deviceBox->setCurrentIndex(defaultDeviceIndex);
        if (defaultDeviceIndex == currentIndex) {
            // device changed, reinitialize audio
            deviceChanged(defaultDeviceIndex);
        }
    }
}



void AudioTest::restartAudioStream()
{
    m_pushTimer->stop();
    // Reset audiosink
    m_audioSink->reset();
    m_generator.reset();

    qreal initialVolume = QAudio::convertVolume(m_audioSink->volume(), QAudio::LinearVolumeScale,
                                                QAudio::LogarithmicVolumeScale);
    m_volumeSlider->setValue(qRound(initialVolume * 100));

    switch (m_mode) {
    case AudioTestMode::Pull:
    case AudioTestMode::Push: {
        // rebuild generator and sink with the requested format
        const int durationSeconds = 1;
        const int toneFrequencyInHz = 600;
        m_generator = std::make_unique<Generator>(m_audioSink->format(), durationSeconds * 1000000,
                                                  toneFrequencyInHz);
        m_generator->start();
        break;
    }
    case AudioTestMode::Callback: {
        // we don't use the QIODevice based generator in callback mode, but capture the oscillator
        break;
    }
    }

    switch (m_mode) {
    case AudioTestMode::Pull: {
        m_audioSink->start(m_generator.get());
        break;
    }
    case AudioTestMode::Push: {
        // push mode: periodically push to QAudioSink using a timer
        auto *io = m_audioSink->start();
        m_pushTimer->disconnect();

        connect(m_pushTimer, &QTimer::timeout, this, [this, io]() {
            if (m_audioSink->state() == QAudio::StoppedState)
                return;

            int len = m_audioSink->bytesFree();
            QByteArray buffer(len, 0);
            len = m_generator->read(buffer.data(), len);
            if (len)
                io->write(buffer.data(), len);
        });

        m_pushTimer->start(10);
        break;
    }
    case AudioTestMode::Callback: {
        auto generator = SineOscillator(600.0f, float(m_audioSink->format().sampleRate()));
        int numberOfChannels = m_audioSink->format().channelCount();
        switch (m_audioSink->format().sampleFormat()) {
        case QAudioFormat::UInt8: {
            m_audioSink->start(
                    [generator, numberOfChannels](QSpan<uint8_t> buffer) mutable -> void {
                while (!buffer.isEmpty()) {
                    float sampleValue = generator.nextSample();
                    QSpan<uint8_t> currentFrame = buffer.first(numberOfChannels);
                    Q_ASSERT(currentFrame.size() == numberOfChannels);
                    std::fill(currentFrame.begin(), currentFrame.end(), asUint8(sampleValue));
                    buffer = buffer.subspan(numberOfChannels);
                }
            });
            break;
        }
        case QAudioFormat::Int16: {
            m_audioSink->start([generator, numberOfChannels](QSpan<qint16> buffer) mutable -> void {
                while (!buffer.isEmpty()) {
                    float sampleValue = generator.nextSample();
                    QSpan<qint16> currentFrame = buffer.first(numberOfChannels);
                    Q_ASSERT(currentFrame.size() == numberOfChannels);
                    std::fill(currentFrame.begin(), currentFrame.end(), asInt16(sampleValue));
                    buffer = buffer.subspan(numberOfChannels);
                }
            });
            break;
        }
        case QAudioFormat::Int32: {
            m_audioSink->start([generator, numberOfChannels](QSpan<qint32> buffer) mutable -> void {
                while (!buffer.isEmpty()) {
                    float sampleValue = generator.nextSample();
                    QSpan<qint32> currentFrame = buffer.first(numberOfChannels);
                    Q_ASSERT(currentFrame.size() == numberOfChannels);
                    std::fill(currentFrame.begin(), currentFrame.end(), asInt32(sampleValue));
                    buffer = buffer.subspan(numberOfChannels);
                }
            });
            break;
        }
        case QAudioFormat::Float: {
            m_audioSink->start([generator, numberOfChannels](QSpan<float> buffer) mutable -> void {
                while (!buffer.isEmpty()) {
                    float sampleValue = generator.nextSample();
                    QSpan<float> currentFrame = buffer.first(numberOfChannels);
                    Q_ASSERT(currentFrame.size() == numberOfChannels);
                    std::fill(currentFrame.begin(), currentFrame.end(), sampleValue);
                    buffer = buffer.subspan(numberOfChannels);
                }
            });
            break;
        }
        default:
            Q_UNREACHABLE();
        }
        break;
    }
    default:
        Q_UNREACHABLE();
    }
}

void AudioTest::toggleSuspendResume()
{
    switch (m_audioSink->state()) {
    case QAudio::SuspendedState:
        m_audioSink->resume();
        return;
    case QAudio::ActiveState:
        m_audioSink->suspend();
        return;

    default:
        return;
    }
}

void AudioTest::cleanupAudioSink()
{
    if (m_audioSink) {
        m_audioSink->stop();
        m_audioSink->disconnect(this);
    }
    m_audioSink.reset();
    m_generator.reset();
    m_currentDevice = {};
}

#include "moc_audiooutput.cpp"
