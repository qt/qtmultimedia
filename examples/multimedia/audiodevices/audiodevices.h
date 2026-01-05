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

class AudioDevices : public AudioDevicesBase
{
    Q_OBJECT

public:
    explicit AudioDevices(QWidget *parent = nullptr);

private:
    QAudioDevice m_deviceInfo;
    QAudioDevice::Mode m_mode = QAudioDevice::Output;
    QMediaDevices *m_devices = nullptr;

    void updateDevicePropertes();

private slots:
    void init();
    void updateAudioDevices();
    void modeChanged(int idx);
    void deviceChanged(int idx);
};

#endif
