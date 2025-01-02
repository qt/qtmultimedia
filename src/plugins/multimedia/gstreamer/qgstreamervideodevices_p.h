// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include <private/qplatformvideodevices_p.h>
#include <gst/gst.h>
#include <qaudiodevice.h>
#include <vector>

QT_BEGIN_NAMESPACE

class QGstreamerVideoDevices : public QPlatformVideoDevices
{
public:
    QGstreamerVideoDevices(QPlatformMediaIntegration *integration);

    QList<QCameraDevice> videoDevices() const override;
    GstDevice *videoDevice(const QByteArray &id) const;

    void addDevice(GstDevice *);
    void removeDevice(GstDevice *);

private:
    struct QGstDevice {
        GstDevice *gstDevice = nullptr;
        QByteArray id;
    };

    quint64 m_idGenerator = 0;
    std::vector<QGstDevice> m_videoSources;
};

QT_END_NAMESPACE

#endif
