// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef AUDIOINPUT_H
#define AUDIOINPUT_H

#include <QAudioSource>
#include <QMediaDevices>

#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QWidget>

#include <QPixmap>
#include <QByteArray>

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

    qreal calculateLevel(const char *data, qint64 len) const;

signals:
    void levelChanged(qreal level);

private:
    const QAudioFormat m_format;
    qreal m_level = 0.0; // 0.0 <= m_level <= 1.0
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

signals:
    void pullModeChanged();

private:
    void initializeWindow();
    void initializeAudio(const QAudioDevice &deviceInfo);
    void initializeErrorWindow();
    void restartAudioStream();

private slots:
    void init();
    void toggleMode();
    void toggleSuspend();
    void deviceChanged(int index);
    void sliderChanged(int value);
    void updateAudioDevices();

private:
    // Owned by layout
    RenderArea *m_canvas = nullptr;
    QPushButton *m_modeButton = nullptr;
    QPushButton *m_suspendResumeButton = nullptr;
    QComboBox *m_deviceBox = nullptr;
    QSlider *m_volumeSlider = nullptr;

    QMediaDevices *m_devices = nullptr;
    std::unique_ptr<AudioInfo> m_audioInfo;
    std::unique_ptr<QAudioSource> m_audioSource;
    bool m_pullMode = true;
};

#endif // AUDIOINPUT_H
