// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERIMAGECAPTURECONTROL_H
#define QGSTREAMERIMAGECAPTURECONTROL_H

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

#include <private/qplatformimagecapture_p.h>
#include <private/qmultimediautils_p.h>
#include "qgstreamermediacapture_p.h"
#include "qgstreamerbufferprobe_p.h"

#include <qqueue.h>

#include <qgst_p.h>
#include <gst/video/video.h>

QT_BEGIN_NAMESPACE

class QGstreamerImageCapture : public QPlatformImageCapture, private QGstreamerBufferProbe

{
    Q_OBJECT
public:
    static QMaybe<QPlatformImageCapture *> create(QImageCapture *parent);
    virtual ~QGstreamerImageCapture();

    bool isReadyForCapture() const override;
    int capture(const QString &fileName) override;
    int captureToBuffer() override;

    QImageEncoderSettings imageSettings() const override;
    void setImageSettings(const QImageEncoderSettings &settings) override;

    bool probeBuffer(GstBuffer *buffer) override;

    void setCaptureSession(QPlatformMediaCaptureSession *session);

    QGstElement gstElement() const { return bin.element(); }

public Q_SLOTS:
    void cameraActiveChanged(bool active);
    void onCameraChanged();

private:
    QGstreamerImageCapture(QGstElement videoconvert, QGstElement jpegenc, QGstElement jifmux,
                           QImageCapture *parent);

    void setResolution(const QSize &resolution);
    int doCapture(const QString &fileName);
    static gboolean saveImageFilter(GstElement *element, GstBuffer *buffer, GstPad *pad, void *appdata);

    QGstreamerMediaCapture *m_session = nullptr;
    int m_lastId = 0;
    QImageEncoderSettings m_settings;

    struct PendingImage {
        int id;
        QString filename;
        QMediaMetaData metaData;
    };

    QQueue<PendingImage> pendingImages;

    QGstBin bin;
    QGstElement queue;
    QGstElement filter;
    QGstElement videoConvert;
    QGstElement encoder;
    QGstElement muxer;
    QGstElement sink;
    QGstPad videoSrcPad;

    bool passImage = false;
    bool cameraActive = false;
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAPTURECORNTROL_H
