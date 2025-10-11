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

#include <QtMultimedia/private/qplatformimagecapture_p.h>
#include <QtMultimedia/private/qmultimediautils_p.h>

#include <QtCore/qmutex.h>
#include <QtCore/qqueue.h>
#include <QtConcurrent/QtConcurrentRun>

#include <common/qgst_p.h>
#include <common/qgstreamerbufferprobe_p.h>
#include <mediacapture/qgstreamermediacapture_p.h>
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

    QGstElement gstElement() const { return bin; }

    void setMetaData(const QMediaMetaData &m) override;

public Q_SLOTS:
    void cameraActiveChanged(bool active);
    void onCameraChanged();

private:
    QGstreamerImageCapture(QImageCapture *parent);

    void setResolution(const QSize &resolution);
    int doCapture(const QString &fileName);
    static gboolean saveImageFilter(GstElement *element, GstBuffer *buffer, GstPad *pad,
                                    QGstreamerImageCapture *capture);

    void saveBufferToImage(GstBuffer *buffer);

    mutable QRecursiveMutex
            m_mutex; // guard all elements accessed from probeBuffer/saveBufferToImage
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

    QGObjectHandlerScopedConnection m_handoffConnection;

    QMap<int, QFuture<void>> m_pendingFutures;
    int futureIDAllocator = 0;

    template <typename Functor>
    void invokeDeferred(Functor &&fn)
    {
        QMetaObject::invokeMethod(this, std::forward<decltype(fn)>(fn), Qt::QueuedConnection);
    };
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAPTURECORNTROL_H
