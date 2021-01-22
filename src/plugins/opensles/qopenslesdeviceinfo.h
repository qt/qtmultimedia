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

#ifndef QOPENSLESDEVICEINFO_H
#define QOPENSLESDEVICEINFO_H

#include <qaudiosystem.h>

QT_BEGIN_NAMESPACE

class QOpenSLESEngine;

class QOpenSLESDeviceInfo : public QAbstractAudioDeviceInfo
{
    Q_OBJECT

public:
    QOpenSLESDeviceInfo(const QByteArray &device, QAudio::Mode mode);
    ~QOpenSLESDeviceInfo() {}

    QAudioFormat preferredFormat() const;
    bool isFormatSupported(const QAudioFormat &format) const;
    QString deviceName() const;
    QStringList supportedCodecs();
    QList<int> supportedSampleRates();
    QList<int> supportedChannelCounts();
    QList<int> supportedSampleSizes();
    QList<QAudioFormat::Endian> supportedByteOrders();
    QList<QAudioFormat::SampleType> supportedSampleTypes();

private:
    QOpenSLESEngine *m_engine;
    QByteArray m_device;
    QAudio::Mode m_mode;
};

QT_END_NAMESPACE

#endif // QOPENSLESDEVICEINFO_H
