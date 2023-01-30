// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef AUDIODEVICES_H
#define AUDIODEVICES_H

#include "ui_audiodevicesbase.h"

#include <QAudioDevice>
#include <QMainWindow>
#include <QMediaDevices>
#include <QObject>

class AudioDevicesBase : public QMainWindow, public Ui::AudioDevicesBase
{
public:
    AudioDevicesBase(QWidget *parent = nullptr);
    virtual ~AudioDevicesBase();
};

class AudioTest : public AudioDevicesBase
{
    Q_OBJECT

public:
    explicit AudioTest(QWidget *parent = nullptr);

private:
    QAudioDevice m_deviceInfo;
    QAudioFormat m_settings;
    QAudioDevice::Mode m_mode = QAudioDevice::Input;
    QMediaDevices *m_devices = nullptr;

private slots:
    void init();
    void updateAudioDevices();
    void modeChanged(int idx);
    void deviceChanged(int idx);
    void sampleRateChanged(int idx);
    void channelChanged(int idx);
    void sampleFormatChanged(int idx);
    void test();
    void populateTable();
};

#endif
