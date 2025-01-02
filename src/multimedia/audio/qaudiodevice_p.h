// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QAUDIODEVICEINFO_P_H
#define QAUDIODEVICEINFO_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/qaudiodevice.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIA_EXPORT QAudioDevicePrivate : public QSharedData
{
public:
    QAudioDevicePrivate(const QByteArray &i, QAudioDevice::Mode m)
        : id(i),
          mode(m)
    {}
    virtual ~QAudioDevicePrivate();
    QByteArray id;
    QAudioDevice::Mode mode = QAudioDevice::Output;
    bool isDefault = false;

    QAudioFormat preferredFormat;
    QString description;
    int minimumSampleRate = 0;
    int maximumSampleRate = 0;
    int minimumChannelCount = 0;
    int maximumChannelCount = 0;
    QList<QAudioFormat::SampleFormat> supportedSampleFormats;
    QAudioFormat::ChannelConfig channelConfiguration = QAudioFormat::ChannelConfigUnknown;

    bool operator == (const QAudioDevicePrivate &other) const
    {
        return id == other.id && mode == other.mode && isDefault == other.isDefault
                && preferredFormat == other.preferredFormat && description == other.description
                && minimumSampleRate == other.minimumSampleRate
                && maximumSampleRate == other.maximumSampleRate
                && minimumChannelCount == other.minimumChannelCount
                && maximumChannelCount == other.maximumChannelCount
                && supportedSampleFormats == other.supportedSampleFormats
                && channelConfiguration == other.channelConfiguration;
    }

    QAudioDevice create() { return QAudioDevice(this); }
};

QT_END_NAMESPACE

#endif // QAUDIODEVICEINFO_H
