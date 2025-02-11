// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//


#ifndef QWINDOWSAUDIODEVICEINFO_H
#define QWINDOWSAUDIODEVICEINFO_H

#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>

#include <QtMultimedia/qaudiodevice.h>
#include <private/qaudiosystem_p.h>
#include <private/qaudiodevice_p.h>
#include <QtCore/private/qcomptr_p.h>

struct IMMDevice;

QT_BEGIN_NAMESPACE

class QWindowsAudioDeviceInfo : public QAudioDevicePrivate
{
public:
    QWindowsAudioDeviceInfo(QByteArray dev,
                            ComPtr<IMMDevice> immdev,
                            QString description,
                            QAudioDevice::Mode mode);
    ~QWindowsAudioDeviceInfo();

    bool open();
    void close();

    ComPtr<IMMDevice> immDev() const { return m_immDev; }

private:
    ComPtr<IMMDevice> m_immDev;
};

QT_END_NAMESPACE


#endif // QWINDOWSAUDIODEVICEINFO_H
