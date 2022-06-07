// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENSLESDEVICEINFO_H
#define QOPENSLESDEVICEINFO_H

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

#include <private/qaudiosystem_p.h>
#include <private/qaudiodevice_p.h>

QT_BEGIN_NAMESPACE

class QOpenSLESEngine;

class QOpenSLESDeviceInfo : public QAudioDevicePrivate
{
public:
    QOpenSLESDeviceInfo(const QByteArray &device, const QString &desc, QAudioDevice::Mode mode);
    ~QOpenSLESDeviceInfo() {}

private:
    QOpenSLESEngine *m_engine;
};

QT_END_NAMESPACE

#endif // QOPENSLESDEVICEINFO_H
