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
#include <QtCore/private/qexpected_p.h>
#include <QtConcurrent/QtConcurrentRun>

#include <common/qgst_p.h>
#include <common/qgstreamerbufferprobe_p.h>
#include <mediacapture/qgstreamermediacapturesession_p.h>
#include <gst/video/video.h>

#include <map>

QT_BEGIN_NAMESPACE

class QGstreamerImageCapture : public QPlatformImageCapture, private QGstreamerBufferProbe
{
    Q_OBJECT

public:
    static q23::expected<QPlatformImageCapture *, QString> create(QImageCapture *parent);
    ~QGstreamerImageCapture() override;

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
    int doCapture(QString fileName);
    void saveBufferToFile(QGstBufferHandle, QString filename, int id);
    void convertBufferToImage(const QMutexLocker<QRecursiveMutex> &, QGstBufferHandle, QGstCaps,
                              QMediaMetaData, int id);

    mutable QRecursiveMutex m_mutex; // guard all elements accessed from gstreamer / worker threads
    QGstreamerMediaCaptureSession *m_session = nullptr;
    int m_lastId = 0;
    QImageEncoderSettings m_settings;

    struct PendingImage {
        int id;
        QString filename;
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

    std::atomic_bool m_captureNextBuffer{};
    bool cameraActive = false;

    QMutex m_pendingFuturesMutex;
    std::map<int, QFuture<void>> m_pendingFutures;
    std::atomic<int> m_futureIDAllocator{};

    template <typename Functor>
    void invokeDeferred(Functor &&fn);

    template <typename Functor>
    void runInThreadPool(Functor);
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAPTURECORNTROL_H
