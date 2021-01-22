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

#ifndef QAUDIODEVICEFACTORY_P_H
#define QAUDIODEVICEFACTORY_P_H

#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>

#include <qtmultimediaglobal.h>
#include <qmultimedia.h>

#include "qaudiodeviceinfo.h"

QT_BEGIN_NAMESPACE


class QAbstractAudioInput;
class QAbstractAudioOutput;
class QAbstractAudioDeviceInfo;

class QAudioDeviceFactory
{
public:
    static QList<QAudioDeviceInfo> availableDevices(QAudio::Mode mode);

    static QAudioDeviceInfo defaultDevice(QAudio::Mode mode);

    static QAbstractAudioDeviceInfo* audioDeviceInfo(const QString &realm, const QByteArray &handle, QAudio::Mode mode);

    static QAbstractAudioInput* createDefaultInputDevice(QAudioFormat const &format);
    static QAbstractAudioOutput* createDefaultOutputDevice(QAudioFormat const &format);

    static QAbstractAudioInput* createInputDevice(QAudioDeviceInfo const &device, QAudioFormat const &format);
    static QAbstractAudioOutput* createOutputDevice(QAudioDeviceInfo const &device, QAudioFormat const &format);

    static QAbstractAudioInput* createNullInput();
    static QAbstractAudioOutput* createNullOutput();
};

QT_END_NAMESPACE

#endif // QAUDIODEVICEFACTORY_P_H

