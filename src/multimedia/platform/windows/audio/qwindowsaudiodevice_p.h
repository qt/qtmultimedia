/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

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
#include <private/qwindowsiupointer_p.h>

struct IMMDevice;

QT_BEGIN_NAMESPACE

const unsigned int MAX_SAMPLE_RATES = 5;
const unsigned int SAMPLE_RATES[] = { 8000, 11025, 22050, 44100, 48000 };

class QWindowsAudioDeviceInfo : public QAudioDevicePrivate
{
public:
    QWindowsAudioDeviceInfo(QByteArray dev, QWindowsIUPointer<IMMDevice> immdev, int waveID, const QString &description, QAudioDevice::Mode mode);
    ~QWindowsAudioDeviceInfo();

    bool open();
    void close();

    bool testSettings(const QAudioFormat& format) const;

    int waveId() const { return devId; }
    QWindowsIUPointer<IMMDevice> immDev() const { return immdev; }

private:
    quint32 devId;
    QWindowsIUPointer<IMMDevice> immdev;
};



QT_END_NAMESPACE


#endif // QWINDOWSAUDIODEVICEINFO_H
