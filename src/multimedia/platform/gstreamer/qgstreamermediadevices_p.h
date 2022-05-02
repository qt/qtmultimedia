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

#ifndef QGSTREAMERMEDIADEVICES_H
#define QGSTREAMERMEDIADEVICES_H

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
#include <gst/gst.h>
#include <qset.h>
#include <qaudiodevice.h>

QT_BEGIN_NAMESPACE

class QGstreamerMediaDevices : public QPlatformMediaDevices
{
public:
    QGstreamerMediaDevices();

    QList<QAudioDevice> audioInputs() const override;
    QList<QAudioDevice> audioOutputs() const override;
    QList<QCameraDevice> videoInputs() const override;
    QPlatformAudioSource *createAudioSource(const QAudioDevice &deviceInfo) override;
    QPlatformAudioSink *createAudioSink(const QAudioDevice &deviceInfo) override;

    void addDevice(GstDevice *);
    void removeDevice(GstDevice *);

    GstDevice *audioDevice(const QByteArray &id, QAudioDevice::Mode mode) const;
    GstDevice *videoDevice(const QByteArray &id) const;

private:
    QSet<GstDevice *> m_videoSources;
    QSet<GstDevice *> m_audioSources;
    QSet<GstDevice *> m_audioSinks;
};

QT_END_NAMESPACE

#endif
