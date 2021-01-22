/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/
#ifndef IOSAUDIODEVICEINFO_H
#define IOSAUDIODEVICEINFO_H

#include <qaudiosystem.h>

#if defined(Q_OS_OSX)
# include <CoreAudio/CoreAudio.h>
#endif

QT_BEGIN_NAMESPACE

class CoreAudioDeviceInfo : public QAbstractAudioDeviceInfo
{
    Q_OBJECT

public:
    CoreAudioDeviceInfo(const QByteArray &device, QAudio::Mode mode);
    ~CoreAudioDeviceInfo() {}

    QAudioFormat preferredFormat() const;
    bool isFormatSupported(const QAudioFormat &format) const;
    QString deviceName() const;
    QStringList supportedCodecs();
    QList<int> supportedSampleRates();
    QList<int> supportedChannelCounts();
    QList<int> supportedSampleSizes();
    QList<QAudioFormat::Endian> supportedByteOrders();
    QList<QAudioFormat::SampleType> supportedSampleTypes();

    static QByteArray defaultDevice(QAudio::Mode mode);
    static QList<QByteArray> availableDevices(QAudio::Mode mode);

private:
#if defined(Q_OS_OSX)
    AudioDeviceID m_deviceId;
#endif

    QString m_device;
    QAudio::Mode m_mode;
};

QT_END_NAMESPACE

#endif
