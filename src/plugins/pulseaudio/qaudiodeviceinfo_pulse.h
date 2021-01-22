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

#ifndef QAUDIODEVICEINFOPULSE_H
#define QAUDIODEVICEINFOPULSE_H

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

#include <QtCore/qbytearray.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qlist.h>

#include "qaudio.h"
#include "qaudiodeviceinfo.h"
#include "qaudiosystem.h"

QT_BEGIN_NAMESPACE

class QPulseAudioDeviceInfo : public QAbstractAudioDeviceInfo
{
    Q_OBJECT

public:
    QPulseAudioDeviceInfo(const QByteArray &device, QAudio::Mode mode);
    ~QPulseAudioDeviceInfo() {}

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
    QByteArray m_device;
    QAudio::Mode m_mode;
};

QT_END_NAMESPACE

#endif

