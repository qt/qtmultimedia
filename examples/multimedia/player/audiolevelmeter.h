// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef AUDIOLEVELMETER_H
#define AUDIOLEVELMETER_H

#include <QWidget>
#include <QThread>
#include <queue>
#include <chrono>
#include <QTimer>
#include <QAudioBuffer>

QT_BEGIN_NAMESPACE
class QToolButton;
class QLabel;
QT_END_NAMESPACE

using namespace std::chrono_literals;
using std::chrono::milliseconds;

// Constants used by AudioLevelMeter and MeterChannel
static constexpr int WIDGET_WIDTH = 34;
static constexpr int MAX_CHANNELS = 8;
static constexpr auto PEAK_COLOR = "#1F9B5D";
static constexpr auto RMS_COLOR = "#28C878";
static constexpr milliseconds RMS_WINDOW = 400ms;
static constexpr milliseconds PEAK_LABEL_HOLD_TIME = 2000ms;
static constexpr milliseconds DECAY_EASE_IN_TIME = 160ms;
static constexpr milliseconds UPDATE_INTERVAL = 16ms; // Assuming 60 Hz refresh rate.
static constexpr float DB_DECAY_PER_SECOND = 20.0f;
static constexpr float DB_DECAY_PER_UPDATE =
        DB_DECAY_PER_SECOND / (1000 / UPDATE_INTERVAL.count());
static constexpr float DB_MAX = 0.0f;
static constexpr float DB_MIN = -60.0f;

// A struct used by BufferAnalyzer to emit its results back to AudioLevelMeter
struct BufferValues {
    BufferValues(int nChannels) : peaks(nChannels, 0.0f), squares(nChannels, 0.0f) {}
    std::vector<float> peaks;
    std::vector<float> squares;
};

// A worker class analyzing incoming buffers on a separate worker thread
class BufferAnalyzer : public QObject
{
    Q_OBJECT
public:
    void requestStop() { m_stopRequested.store(true); }

public slots:
    void analyzeBuffer(const QAudioBuffer &buffer, int maxChannelsToAnalyze);

signals:
    void valuesReady(const BufferValues &values);

private:
    std::atomic<bool> m_stopRequested = false;
};

// A custom QWidget representing an audio channel in the audio level meter. It serves
// both as a model for the channels's peak and RMS values and as a view using the overrided
// paintEvent()
class MeterChannel : public QWidget
{
    Q_OBJECT
    friend class AudioLevelMeter;

private:
    explicit MeterChannel(QWidget *parent = nullptr);

    void paintEvent(QPaintEvent *event) override;
    void activate();
    void deactivate();
    void updatePeak(float peak);
    void updateRms(float sumOfSquaresForOneBuffer, milliseconds duration, int frameCount);
    void clearRmsData();
    void decayPeak();
    void decayRms();
    float normalize(float dB);

    float m_peakDecayRate = 0.0f;
    float m_rmsDecayRate = 0.0f;
    float m_peak = DB_MIN;
    float m_rms = DB_MIN;
    float m_sumOfSquares = 0.0f;
    std::queue<float> m_sumOfSquaresQueue{};
    QBrush m_peakBrush{PEAK_COLOR};
    QBrush m_rmsBrush{RMS_COLOR};
};

// The audio level meter´s parent widget class. It acts as a controller
// for the MeterChannel widgets and the BufferAnalyzer worker.
class AudioLevelMeter : public QWidget
{
    Q_OBJECT

public:
    explicit AudioLevelMeter(QWidget *parent = nullptr);
    ~AudioLevelMeter() {
        m_analyzerThread.requestInterruption();
        m_bufferAnalyzer->requestStop();
        m_analyzerThread.quit();
        m_analyzerThread.wait();
    };

signals:
    void newBuffer(const QAudioBuffer &buffer, int nChannels);

public slots:
    void onAudioBufferReceived(const QAudioBuffer &buffer);
    void deactivate();

private slots:
    void toggleOnOff();
    void updateValues(const BufferValues &values);
    void updateBars();
    void resetPeakLabel();

private:
    void activate();
    void updateChannelCount(int nCh);
    void updatePeakLabel(float peak);
    void clearAllRmsData();

    bool m_isOn = true;
    bool m_isActive = false;
    QList<MeterChannel *> m_channels{MAX_CHANNELS};
    int m_channelCount = 0;
    milliseconds m_bufferDurationMs = 0ms;
    int m_frameCount = 0;
    float m_highestPeak = 0.0f;

    QTimer m_updateTimer;
    QTimer m_deactivationTimer;
    QTimer m_peakLabelHoldTimer;
    QLabel *m_peakLabel = nullptr;
    QToolButton *m_onOffButton = nullptr;
    BufferAnalyzer *m_bufferAnalyzer = nullptr;
    QThread m_analyzerThread;
};

#endif // AUDIOLEVELMETER_H
