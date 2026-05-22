// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef AUDIOINPUT_H
#define AUDIOINPUT_H

#include "audiorecorder.h"

#include <QAudioSource>
#include <QMediaDevices>

#include <QBasicTimer>

#include <QComboBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

#include <QPixmap>
#include <QByteArray>

#include <atomic>
#include <memory>

enum class AudioTestMode : uint8_t {
    Pull,
    Push,
    Callback,
};

class AudioInfo : public QIODevice
{
    Q_OBJECT

public:
    AudioInfo(const QAudioFormat &format);

    void start();
    void stop();

    [[nodiscard]] float level() const { return m_level; }

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

signals:
    void levelChanged(float level);

private:
    const QAudioFormat m_format;
    float m_level = 0.f; // 0.0 <= m_level <= 1.0
};

class RenderArea : public QWidget
{
    Q_OBJECT

public:
    explicit RenderArea(QWidget *parent = nullptr);

    void setLevel(float value);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    float m_level = 0.f;
};

class InputTest final : public QWidget
{
    Q_OBJECT

public:
    InputTest();
    ~InputTest() override;

private:
    void initializeWindow();
    void startAudioSource(const QAudioDevice &, const QAudioFormat &);
    void cleanupAudioSource();
    void initializeErrorWindow();
    void restartAudioStream(bool record);
    void timerEvent(QTimerEvent *) override;

    template <typename T>
    void processCallback(QSpan<const T> buffer, const QAudioFormat &format)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(clang::nonblocking)
            [[clang::nonblocking]]
#endif
            ;

    void startPullMode(bool record);
    void startPushMode(bool record);
    void startCallbackMode(bool record);

    QString getRecordingFileName() const;

private:
    void init();
    void toggleSuspend();
    void deviceChanged(int index);
    void sliderChanged(int value);
    void updateAudioDevices();
    void formatChanged(QComboBox *box);
    void updateRecordingProgress();
    void updateControlsForRecording();
    void setRecorder(std::unique_ptr<AudioRecorder> recorder);

private:
    QVBoxLayout *m_layout = nullptr;
    // Owned by layout
    RenderArea *m_canvas = nullptr;
    QComboBox *m_modeBox = nullptr;
    QPushButton *m_suspendResumeButton = nullptr;
    QComboBox *m_deviceBox = nullptr;
    QComboBox *m_periodBox = nullptr;
    QSlider *m_volumeSlider = nullptr;
    QComboBox *m_formatBox = nullptr;
    QComboBox *m_rateBox = nullptr;
    QComboBox *m_channelsBox = nullptr;
    QPushButton *m_recordButton = nullptr;
    QProgressBar *m_recordProgressBar = nullptr;

    QMediaDevices *m_devices = nullptr;
    QAudioDevice m_currentDevice;
    std::unique_ptr<AudioInfo> m_audioInfo;
    std::unique_ptr<QAudioSource> m_audioSource;
    AudioTestMode m_mode = AudioTestMode::Pull;

    QBasicTimer m_callbackVisualizerTimer;
    QBasicTimer m_recordingProgressTimer;
    std::atomic<float> m_level = 0.f;

    std::unique_ptr<AudioRecorder> m_recorder;
};

#endif // AUDIOINPUT_H
