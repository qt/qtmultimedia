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
