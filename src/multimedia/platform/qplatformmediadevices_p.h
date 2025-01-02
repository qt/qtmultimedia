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
#include <qobject.h>
#include <mutex>

QT_BEGIN_NAMESPACE

class QMediaDevices;
class QAudioDevice;
class QCameraDevice;
class QPlatformAudioSource;
class QPlatformAudioSink;
class QAudioFormat;
class QPlatformMediaIntegration;

class Q_MULTIMEDIA_EXPORT QPlatformMediaDevices : public QObject
{
    Q_OBJECT
public:
    QPlatformMediaDevices();
    ~QPlatformMediaDevices() override;

    static void setDevices(QPlatformMediaDevices *);
    static QPlatformMediaDevices *instance();

    virtual QList<QAudioDevice> audioInputs() const;
    virtual QList<QAudioDevice> audioOutputs() const;

    virtual QPlatformAudioSource *createAudioSource(const QAudioDevice &, QObject *parent);
    virtual QPlatformAudioSink *createAudioSink(const QAudioDevice &, QObject *parent);

    QPlatformAudioSource *audioInputDevice(const QAudioFormat &format,
                                           const QAudioDevice &deviceInfo, QObject *parent);
    QPlatformAudioSink *audioOutputDevice(const QAudioFormat &format,
                                          const QAudioDevice &deviceInfo, QObject *parent);

    virtual void prepareAudio();

    void initVideoDevicesConnection();

Q_SIGNALS:
    void audioInputsChanged();
    void audioOutputsChanged();
    void videoInputsChanged();

private:
    std::once_flag m_videoDevicesConnectionFlag;
};

QT_END_NAMESPACE


#endif // QPLATFORMMEDIADEVICES_H
