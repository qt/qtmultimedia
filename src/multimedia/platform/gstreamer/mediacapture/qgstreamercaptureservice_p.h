/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGSTREAMERCAPTURESERVICE_H
#define QGSTREAMERCAPTURESERVICE_H

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

#include <private/qmediaplatformcaptureinterface_p.h>
#include <private/qmediaplatformintegration_p.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE
class QVideoDeviceSelectorControl;

class QGstreamerAudioProbeControl;
class QGstreamerCaptureSession;
class QGstreamerCameraControl;
class QGstreamerMessage;
class QGstreamerBusHelper;
class QGstreamerVideoRenderer;
class QGstreamerVideoWindow;
class QGstreamerElementFactory;
class QGstreamerCaptureMetaDataControl;
class QGstreamerImageCaptureControl;
class QGstreamerV4L2Input;

class QGstreamerCaptureService : public QMediaPlatformCaptureInterface
{
    Q_OBJECT

public:
    QGstreamerCaptureService(QMediaPlatformIntegration::CaptureMode mode);
    virtual ~QGstreamerCaptureService();

    QObject *requestControl(const char *name) override;
    void releaseControl(QObject *) override;

private:
    void setAudioPreview(GstElement *);

    QGstreamerCaptureSession *m_captureSession;
    QGstreamerCameraControl *m_cameraControl;
#if defined(USE_GSTREAMER_CAMERA)
    QGstreamerV4L2Input *m_videoInput;
#endif
    QGstreamerCaptureMetaDataControl *m_metaDataControl;

    QVideoDeviceSelectorControl *m_videoInputDevice;

    QObject *m_videoOutput;

    QGstreamerVideoRenderer *m_videoRenderer;
    QGstreamerVideoWindow *m_videoWindow;
    QGstreamerImageCaptureControl *m_imageCaptureControl;

    QGstreamerAudioProbeControl *m_audioProbeControl;
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAPTURESERVICE_H
