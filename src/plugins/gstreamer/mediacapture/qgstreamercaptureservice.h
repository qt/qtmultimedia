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

#ifndef QGSTREAMERCAPTURESERVICE_H
#define QGSTREAMERCAPTURESERVICE_H

#include <qmediaservice.h>
#include <qmediacontrol.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE
class QAudioInputSelectorControl;
class QVideoDeviceSelectorControl;

class QGstreamerAudioProbeControl;
class QGstreamerCaptureSession;
class QGstreamerCameraControl;
class QGstreamerMessage;
class QGstreamerBusHelper;
class QGstreamerVideoRenderer;
class QGstreamerVideoWindow;
class QGstreamerVideoWidgetControl;
class QGstreamerElementFactory;
class QGstreamerCaptureMetaDataControl;
class QGstreamerImageCaptureControl;
class QGstreamerV4L2Input;

class QGstreamerCaptureService : public QMediaService
{
    Q_OBJECT

public:
    QGstreamerCaptureService(const QString &service, QObject *parent = 0);
    virtual ~QGstreamerCaptureService();

    QMediaControl *requestControl(const char *name) override;
    void releaseControl(QMediaControl *) override;

private:
    void setAudioPreview(GstElement *);

    QGstreamerCaptureSession *m_captureSession;
    QGstreamerCameraControl *m_cameraControl;
#if defined(USE_GSTREAMER_CAMERA)
    QGstreamerV4L2Input *m_videoInput;
#endif
    QGstreamerCaptureMetaDataControl *m_metaDataControl;

    QAudioInputSelectorControl *m_audioInputSelector;
    QVideoDeviceSelectorControl *m_videoInputDevice;

    QMediaControl *m_videoOutput;

    QGstreamerVideoRenderer *m_videoRenderer;
    QGstreamerVideoWindow *m_videoWindow;
#if defined(HAVE_WIDGETS)
    QGstreamerVideoWidgetControl *m_videoWidgetControl;
#endif
    QGstreamerImageCaptureControl *m_imageCaptureControl;

    QGstreamerAudioProbeControl *m_audioProbeControl;
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAPTURESERVICE_H
