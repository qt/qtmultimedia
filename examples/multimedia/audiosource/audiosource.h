// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef AUDIOINPUT_H
#define AUDIOINPUT_H

#include <QAudioSource>
#include <QMediaDevices>

#include <QBasicTimer>

#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QWidget>

#include <QPixmap>
#include <QByteArray>

#include <atomic>
#include <memory>

class AudioInfo : public QIODevice
{
    Q_OBJECT

public:
    AudioInfo(const QAudioFormat &format);

    void start();
    void stop();

    qreal level() const { return m_level; }

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

signals:
    void levelChanged(qreal level);

private:
    const QAudioFormat m_format;
    qreal m_level = 0.0; // 0.0 <= m_level <= 1.0
};

enum class AudioTestMode {
    Pull,
    Push,
    Callback,
};

class RenderArea : public QWidget
{
    Q_OBJECT

public:
    explicit RenderArea(QWidget *parent = nullptr);

    void setLevel(qreal value);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    qreal m_level = 0;
};

class InputTest : public QWidget
{
    Q_OBJECT

public:
    InputTest();

private:
    void initializeWindow();
    void startAudioSource(const QAudioDevice &, const QAudioFormat &);
    void cleanupAudioSource();
    void initializeErrorWindow();
    void restartAudioStream();
    void timerEvent(QTimerEvent *) override;

    template <typename T>
    void processCallback(QSpan<const T> buffer, const QAudioFormat &format);

private slots:
    void init();
    void toggleSuspend();
    void deviceChanged(int index);
    void sliderChanged(int value);
    void updateAudioDevices();
    void formatChanged(QComboBox *box);

private:
    // Owned by layout
    RenderArea *m_canvas = nullptr;
    QComboBox *m_modeBox = nullptr;
    QPushButton *m_suspendResumeButton = nullptr;
    QComboBox *m_deviceBox = nullptr;
    QSlider *m_volumeSlider = nullptr;
    QComboBox *m_formatBox = nullptr;
    QComboBox *m_rateBox = nullptr;
    QComboBox *m_channelsBox = nullptr;

    QMediaDevices *m_devices = nullptr;
    QAudioDevice m_currentDevice;
    std::unique_ptr<AudioInfo> m_audioInfo;
    std::unique_ptr<QAudioSource> m_audioSource;
    AudioTestMode m_mode = AudioTestMode::Pull;

    QBasicTimer m_callbackVisualizerTimer;
    std::atomic<float> m_level = 0.f;
};

#endif // AUDIOINPUT_H
