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

#ifndef QDARWINMEDIADEVICES_H
#define QDARWINMEDIADEVICES_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qplatformmediadevices_p.h>
#include <qelapsedtimer.h>
#include <qcameradevice.h>

Q_FORWARD_DECLARE_OBJC_CLASS(NSObject);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDeviceDiscoverySession);

QT_BEGIN_NAMESPACE

class QCameraDevice;

class QDarwinMediaDevices : public QPlatformMediaDevices
{
public:
    QDarwinMediaDevices();
    ~QDarwinMediaDevices();

    QList<QAudioDevice> audioInputs() const override;
    QList<QAudioDevice> audioOutputs() const override;
    QList<QCameraDevice> videoInputs() const override;
    QPlatformAudioSource *createAudioSource(const QAudioDevice &info) override;
    QPlatformAudioSink *createAudioSink(const QAudioDevice &info) override;

    void updateCameraDevices();
    void updateAudioDevices();

private:
    QList<QCameraDevice> m_cameraDevices;
    QList<QAudioDevice> m_audioInputs;
    QList<QAudioDevice> m_audioOutputs;

    NSObject *m_deviceConnectedObserver;
    NSObject *m_deviceDisconnectedObserver;
#ifdef Q_OS_MACOS
    void *m_audioDevicesProperty;
#endif
};

QT_END_NAMESPACE

#endif
