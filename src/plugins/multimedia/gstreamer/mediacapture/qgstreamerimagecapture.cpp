// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamerimagecapture_p.h"
#include <private/qplatformcamera_p.h>
#include <private/qplatformimagecapture_p.h>
#include <qgstvideobuffer_p.h>
#include <qgstutils_p.h>
#include <qgstreamermetadata_p.h>
#include <qvideoframeformat.h>
#include <private/qmediastoragelocation_p.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <qstandardpaths.h>

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcImageCapture, "qt.multimedia.imageCapture")

QGstreamerImageCapture::QGstreamerImageCapture(QImageCapture *parent)
  : QPlatformImageCapture(parent),
    QGstreamerBufferProbe(ProbeBuffers)
{
    bin = QGstBin("imageCaptureBin");

    queue = QGstElement("queue", "imageCaptureQueue");
    // configures the queue to be fast, lightweight and non blocking
    queue.set("leaky", 2 /*downstream*/);
    queue.set("silent", true);
    queue.set("max-size-buffers", uint(1));
    queue.set("max-size-bytes", uint(0));
    queue.set("max-size-time", quint64(0));

    videoConvert = QGstElement("videoconvert", "imageCaptureConvert");
    encoder = QGstElement("jpegenc", "jpegEncoder");
    muxer = QGstElement("jifmux", "jpegMuxer");
    sink = QGstElement("fakesink","imageCaptureSink");
    // imageCaptureSink do not wait for a preroll buffer when going READY -> PAUSED
    // as no buffer will arrive until capture() is called
    sink.set("async", false);

    bin.add(queue, videoConvert, encoder, muxer, sink);
    queue.link(videoConvert, encoder, muxer, sink);
    bin.addGhostPad(queue, "sink");

    addProbeToPad(queue.staticPad("src").pad(), false);

    sink.set("signal-handoffs", true);
    g_signal_connect(sink.object(), "handoff", G_CALLBACK(&QGstreamerImageCapture::saveImageFilter), this);
}

QGstreamerImageCapture::~QGstreamerImageCapture()
{
    bin.setStateSync(GST_STATE_NULL);
}

bool QGstreamerImageCapture::isReadyForCapture() const
{
    return m_session && !passImage && cameraActive;
}

int QGstreamerImageCapture::capture(const QString &fileName)
{
    QString path = QMediaStorageLocation::generateFileName(fileName, QStandardPaths::PicturesLocation, QLatin1String("jpg"));
    return doCapture(path);
}

int QGstreamerImageCapture::captureToBuffer()
{
    return doCapture(QString());
}

int QGstreamerImageCapture::doCapture(const QString &fileName)
{
    qCDebug(qLcImageCapture) << "do capture";
    if (!m_session) {
        //emit error in the next event loop,
        //so application can associate it with returned request id.
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                  Q_ARG(int, -1),
                                  Q_ARG(int, QImageCapture::ResourceError),
                                  Q_ARG(QString, QPlatformImageCapture::msgImageCaptureNotSet()));

        qCDebug(qLcImageCapture) << "error 1";
        return -1;
    }
    if (!m_session->camera()) {
        //emit error in the next event loop,
        //so application can associate it with returned request id.
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                  Q_ARG(int, -1),
                                  Q_ARG(int, QImageCapture::ResourceError),
                                  Q_ARG(QString,tr("No camera available.")));

        qCDebug(qLcImageCapture) << "error 2";
        return -1;
    }
    if (passImage) {
        //emit error in the next event loop,
        //so application can associate it with returned request id.
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                  Q_ARG(int, -1),
                                  Q_ARG(int, QImageCapture::NotReadyError),
                                  Q_ARG(QString, QPlatformImageCapture::msgCameraNotReady()));

        qCDebug(qLcImageCapture) << "error 3";
        return -1;
    }
    m_lastId++;

    pendingImages.enqueue({m_lastId, fileName, QMediaMetaData{}});
    // let one image pass the pipeline
    passImage = true;

    emit readyForCaptureChanged(false);
    return m_lastId;
}

bool QGstreamerImageCapture::probeBuffer(GstBuffer *buffer)
{
    if (!passImage)
        return false;
    qCDebug(qLcImageCapture) << "probe buffer";

    passImage = false;

    emit readyForCaptureChanged(isReadyForCapture());

    QGstCaps caps = gst_pad_get_current_caps(bin.staticPad("sink").pad());
    GstVideoInfo previewInfo;
    gst_video_info_from_caps(&previewInfo, caps.get());

    auto memoryFormat = caps.memoryFormat();
    auto fmt = QGstCaps(caps).formatForCaps(&previewInfo);
    auto *sink = m_session->gstreamerVideoSink();
    auto *gstBuffer = new QGstVideoBuffer(buffer, previewInfo, sink, fmt, memoryFormat);
    QVideoFrame frame(gstBuffer, fmt);
    QImage img = frame.toImage();
    if (img.isNull()) {
        qDebug() << "received a null image";
        return true;
    }

    auto &imageData = pendingImages.head();

    emit imageExposed(imageData.id);

    qCDebug(qLcImageCapture) << "Image available!";
    emit imageAvailable(imageData.id, frame);

    emit imageCaptured(imageData.id, img);

    QMediaMetaData metaData = this->metaData();
    metaData.insert(QMediaMetaData::Date, QDateTime::currentDateTime());
    metaData.insert(QMediaMetaData::Resolution, frame.size());
    imageData.metaData = metaData;

    // ensure taginject injects this metaData
    const auto &md = static_cast<const QGstreamerMetaData &>(metaData);
    md.setMetaData(muxer.element());

    emit imageMetadataAvailable(imageData.id, metaData);

    return true;
}

void QGstreamerImageCapture::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QGstreamerMediaCapture *captureSession = static_cast<QGstreamerMediaCapture *>(session);
    if (m_session == captureSession)
        return;

    bool readyForCapture = isReadyForCapture();
    if (m_session) {
        disconnect(m_session, nullptr, this, nullptr);
        m_lastId = 0;
        pendingImages.clear();
        passImage = false;
        cameraActive = false;
    }

    m_session = captureSession;
    if (!m_session) {
        if (readyForCapture)
            emit readyForCaptureChanged(false);
        return;
    }

    connect(m_session, &QPlatformMediaCaptureSession::cameraChanged, this, &QGstreamerImageCapture::onCameraChanged);
    onCameraChanged();
}

void QGstreamerImageCapture::cameraActiveChanged(bool active)
{
    qCDebug(qLcImageCapture) << "cameraActiveChanged" << cameraActive << active;
    if (cameraActive == active)
        return;
    cameraActive = active;
    qCDebug(qLcImageCapture) << "isReady" << isReadyForCapture();
    emit readyForCaptureChanged(isReadyForCapture());
}

void QGstreamerImageCapture::onCameraChanged()
{
    if (m_session->camera()) {
        cameraActiveChanged(m_session->camera()->isActive());
        connect(m_session->camera(), &QPlatformCamera::activeChanged, this, &QGstreamerImageCapture::cameraActiveChanged);
    } else {
        cameraActiveChanged(false);
    }
}

gboolean QGstreamerImageCapture::saveImageFilter(GstElement *element,
                                                       GstBuffer *buffer,
                                                       GstPad *pad,
                                                       void *appdata)
{
    Q_UNUSED(element);
    Q_UNUSED(pad);
    QGstreamerImageCapture *capture = static_cast<QGstreamerImageCapture *>(appdata);

    capture->passImage = false;

    if (capture->pendingImages.isEmpty()) {
        return true;
    }

    auto imageData = capture->pendingImages.dequeue();
    if (imageData.filename.isEmpty()) {
        return true;
    }

    qCDebug(qLcImageCapture) << "saving image as" << imageData.filename;

    QFile f(imageData.filename);
    if (f.open(QFile::WriteOnly)) {
        GstMapInfo info;
        if (gst_buffer_map(buffer, &info, GST_MAP_READ)) {
            f.write(reinterpret_cast<const char *>(info.data), info.size);
            gst_buffer_unmap(buffer, &info);
        }
        f.close();

        static QMetaMethod savedSignal = QMetaMethod::fromSignal(&QGstreamerImageCapture::imageSaved);
        savedSignal.invoke(capture,
                           Qt::QueuedConnection,
                           Q_ARG(int, imageData.id),
                           Q_ARG(QString, imageData.filename));
    } else {
        qCDebug(qLcImageCapture) << "   could not open image file for writing";
    }

    return TRUE;
}

QImageEncoderSettings QGstreamerImageCapture::imageSettings() const
{
    return m_settings;
}

void QGstreamerImageCapture::setImageSettings(const QImageEncoderSettings &settings)
{
    if (m_settings != settings) {
        m_settings = settings;
        // ###
    }
}

QT_END_NAMESPACE
