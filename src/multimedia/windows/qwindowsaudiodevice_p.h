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
#include <QtCore/qstringlist.h>
#include <QtCore/qlist.h>
#include <QtCore/qdebug.h>

#include <QtMultimedia/qaudiodevice.h>
#include <private/qaudiosystem_p.h>
#include <private/qaudiodevice_p.h>
#include <qcomptr_p.h>

struct IMMDevice;

QT_BEGIN_NAMESPACE

const unsigned int MAX_SAMPLE_RATES = 5;
const unsigned int SAMPLE_RATES[] = { 8000, 11025, 22050, 44100, 48000 };

class QWindowsAudioDeviceInfo : public QAudioDevicePrivate
{
public:
    QWindowsAudioDeviceInfo(QByteArray dev, ComPtr<IMMDevice> immdev, int waveID, const QString &description, QAudioDevice::Mode mode);
    ~QWindowsAudioDeviceInfo();

    bool open();
    void close();

    bool testSettings(const QAudioFormat& format) const;

    int waveId() const { return m_devId; }
    ComPtr<IMMDevice> immDev() const { return m_immDev; }

private:
    quint32 m_devId;
    ComPtr<IMMDevice> m_immDev;
};



QT_END_NAMESPACE


#endif // QWINDOWSAUDIODEVICEINFO_H
