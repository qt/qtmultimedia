/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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

#ifndef QNXAUDIODEVICEINFO_H
#define QNXAUDIODEVICEINFO_H

#include "qaudiosystem.h"

QT_BEGIN_NAMESPACE

class QnxAudioDeviceInfo : public QAbstractAudioDeviceInfo
{
    Q_OBJECT

public:
    QnxAudioDeviceInfo(const QString &deviceName, QAudio::Mode mode);
    ~QnxAudioDeviceInfo();

    QAudioFormat preferredFormat() const override;
    bool isFormatSupported(const QAudioFormat &format) const override;
    QString deviceName() const override;
    QStringList supportedCodecs() override;
    QList<int> supportedSampleRates() override;
    QList<int> supportedChannelCounts() override;
    QList<int> supportedSampleSizes() override;
    QList<QAudioFormat::Endian> supportedByteOrders() override;
    QList<QAudioFormat::SampleType> supportedSampleTypes() override;

private:
    const QString m_name;
    const QAudio::Mode m_mode;
};

QT_END_NAMESPACE

#endif
