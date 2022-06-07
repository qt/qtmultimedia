// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMMEDIADEVICES_H
#define QPLATFORMMEDIADEVICES_H

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

#include <private/qtmultimediaglobal_p.h>
#include <qlist.h>

QT_BEGIN_NAMESPACE

class QMediaDevices;
class QAudioDevice;
class QCameraDevice;
class QPlatformAudioSource;
class QPlatformAudioSink;
class QAudioFormat;
class QPlatformMediaIntegration;

class Q_MULTIMEDIA_EXPORT QPlatformMediaDevices
{
public:
    QPlatformMediaDevices();
    virtual ~QPlatformMediaDevices();

    static void setDevices(QPlatformMediaDevices *);
    static QPlatformMediaDevices *instance();

    virtual QList<QAudioDevice> audioInputs() const = 0;
    virtual QList<QAudioDevice> audioOutputs() const = 0;
    virtual QList<QCameraDevice> videoInputs() const;
    virtual QPlatformAudioSource *createAudioSource(const QAudioDevice &deviceInfo) = 0;
    virtual QPlatformAudioSink *createAudioSink(const QAudioDevice &deviceInfo) = 0;

    QPlatformAudioSource *audioInputDevice(const QAudioFormat &format, const QAudioDevice &deviceInfo);
    QPlatformAudioSink *audioOutputDevice(const QAudioFormat &format, const QAudioDevice &deviceInfo);

    void addMediaDevices(QMediaDevices *m)
    {
        m_devices.append(m);
    }
    void removeMediaDevices(QMediaDevices *m)
    {
        m_devices.removeAll(m);
    }

    QList<QMediaDevices *> allMediaDevices() const { return m_devices; }

    void videoInputsChanged() const;

protected:
    void audioInputsChanged() const;
    void audioOutputsChanged() const;

    QList<QMediaDevices *> m_devices;
};

QT_END_NAMESPACE


#endif // QPLATFORMMEDIADEVICES_H
