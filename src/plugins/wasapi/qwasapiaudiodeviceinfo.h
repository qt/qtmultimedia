/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
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

#ifndef QWASAPIAUDIODEVICEINFO_H
#define QWASAPIAUDIODEVICEINFO_H

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QStringList>
#include <QtMultimedia/QAbstractAudioDeviceInfo>
#include <QtMultimedia/QAudio>
#include <QtMultimedia/QAudioFormat>

#include <wrl.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcMmDeviceInfo)

class AudioInterface;
class QWasapiAudioDeviceInfo : public QAbstractAudioDeviceInfo
{
    Q_OBJECT
public:
    explicit QWasapiAudioDeviceInfo(QByteArray dev,QAudio::Mode mode);
    ~QWasapiAudioDeviceInfo();

    QAudioFormat preferredFormat() const override;
    bool isFormatSupported(const QAudioFormat& format) const override;
    QString deviceName() const override;
    QStringList supportedCodecs() override;
    QList<int> supportedSampleRates() override;
    QList<int> supportedChannelCounts() override;
    QList<int> supportedSampleSizes() override;
    QList<QAudioFormat::Endian> supportedByteOrders() override;
    QList<QAudioFormat::SampleType> supportedSampleTypes() override;

private:
    Microsoft::WRL::ComPtr<AudioInterface> m_interface;
    QString m_deviceName;
    QList<int> m_sampleRates;
    QList<int> m_channelCounts;
    QList<int> m_sampleSizes;
    QList<QAudioFormat::SampleType> m_sampleTypes;
};

QT_END_NAMESPACE

#endif // QWASAPIAUDIODEVICEINFO_H
