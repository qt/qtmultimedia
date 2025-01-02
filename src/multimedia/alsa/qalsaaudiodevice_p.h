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


#ifndef QALSAAUDIODEVICEINFO_H
#define QALSAAUDIODEVICEINFO_H

#include <alsa/asoundlib.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qlist.h>
#include <QtCore/qdebug.h>

#include <QtMultimedia/qaudio.h>
#include <private/qaudiodevice_p.h>
#include <private/qaudiosystem_p.h>

QT_BEGIN_NAMESPACE


class QAlsaAudioDeviceInfo : public QAudioDevicePrivate
{
public:
    QAlsaAudioDeviceInfo(const QByteArray &dev, const QString &description, QAudioDevice::Mode mode);
    ~QAlsaAudioDeviceInfo();

private:
    void checkSurround();
    bool surround40;
    bool surround51;
    bool surround71;
};

QT_END_NAMESPACE


#endif // QALSAAUDIODEVICEINFO_H
