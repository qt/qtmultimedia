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

#include "qgstreamerimagecapture_p.h"
#include "qplatformcamera_p.h"
#include <private/qplatformimagecapture_p.h>
#include <private/qgstvideobuffer_p.h>
#include <private/qgstutils_p.h>
#include <private/qgstreamermetadata_p.h>
#include <qvideoframeformat.h>

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
    queue.set("max-size-buffers", 1);
    queue.set("max-size-bytes", 0);
    queue.set("max-size-time", 0);

    videoConvert = QGstElement("videoconvert", "imageCaptureConvert");
    encoder = QGstElement("jpegenc", "jpegEncoder");
    muxer = QGstElement("jifmux", "jpegMuxer");
    sink = QGstElement("fakesink","imageCaptureSink");
    bin.add(queue, videoConvert, encoder, muxer, sink);
    queue.link(videoConvert, encoder, muxer, sink);
    bin.addGhostPad(queue, "sink");
    bin.lockState(true);

    addProbeToPad(queue.staticPad("src").pad(), false);

    sink.set("signal-handoffs", true);
    g_signal_connect(sink.object(), "handoff", G_CALLBACK(&QGstreamerImageCapture::saveImageFilter), this);
}

QGstreamerImageCapture::~QGstreamerImageCapture()
{
    if (m_session)
        m_session->releaseVideoPad(videoSrcPad);
}

bool QGstreamerImageCapture::isReadyForCapture() const
{
    return m_session && !passImage && cameraActive;
}


static QDir defaultDir()
{
    QStringList dirCandidates;

    dirCandidates << QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    dirCandidates << QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    dirCandidates << QDir::homePath();
    dirCandidates << QDir::currentPath();
    dirCandidates << QDir::tempPath();

    for (const QString &path : qAsConst(dirCandidates)) {
        QDir dir(path);
        if (dir.exists() && QFileInfo(path).isWritable())
            return dir;
    }

    return QDir();
}

QString generateFileName(const QDir &dir, const QString &ext)
{
    int lastClip = 0;
    const auto list = dir.entryList(QStringList() << QString::fromLatin1("img_*.%1").arg(ext));
    for (const QString &fileName : list) {
        int imgNumber = QStringView{fileName}.mid(5, fileName.size()-6-ext.length()).toInt();
        lastClip = qMax(lastClip, imgNumber);
    }

    QString name = QString::fromLatin1("img_%1.%2")
                       .arg(lastClip+1,
                            4, //fieldWidth
                            10,
                            QLatin1Char('0'))
                       .arg(ext);

    return dir.absoluteFilePath(name);
}


int QGstreamerImageCapture::capture(const QString &fileName)
{
    QString path = fileName;
    if (path.isEmpty())
        path = generateFileName(defaultDir(), QLatin1String("jpg"));

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
        QMetaObject::invokeMethod(this, "errorOccurred", Qt::QueuedConnection,
                                  Q_ARG(int, -1),
                                  Q_ARG(int, QImageCapture::ResourceError),
                                  Q_ARG(QString, QPlatformImageCapture::msgImageCaptureNotSet()));

        qCDebug(qLcImageCapture) << "error 1";
        return -1;
    }
    if (!m_session->camera()) {
        //emit error in the next event loop,
        //so application can associate it with returned request id.
        QMetaObject::invokeMethod(this, "errorOccurred", Qt::QueuedConnection,
                                  Q_ARG(int, -1),
                                  Q_ARG(int, QImageCapture::ResourceError),
                                  Q_ARG(QString,tr("No camera available.")));

        qCDebug(qLcImageCapture) << "error 2";
        return -1;
    }
    if (passImage) {
        //emit error in the next event loop,
        //so application can associate it with returned request id.
        QMetaObject::invokeMethod(this, "errorOccurred", Qt::QueuedConnection,
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

    link();

    gstPipeline.dumpGraph("captureImage");

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

    GstCaps *caps = gst_pad_get_current_caps(bin.staticPad("sink").pad());
    GstVideoInfo previewInfo;
    gst_video_info_from_caps(&previewInfo, caps);

    auto *gstBuffer = new QGstVideoBuffer(buffer, previewInfo);
    auto fmt = QGstUtils::formatForCaps(caps, &previewInfo);
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

    if (m_session) {
        disconnect(m_session, nullptr, this, nullptr);
        m_lastId = 0;
        pendingImages.clear();
        passImage = false;
        cameraActive = false;
        gstPipeline.beginConfig();
        bin.setStateSync(GST_STATE_NULL);
        gstPipeline.remove(bin);
        gstPipeline.endConfig();
        gstPipeline = {};
    }

    m_session = captureSession;
    if (!m_session)
        return;

    gstPipeline = captureSession->pipeline();
    gstPipeline.beginConfig();
    gstPipeline.add(bin);
    bin.setStateSync(GST_STATE_READY);
    gstPipeline.endConfig();
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

    if (capture->pendingImages.isEmpty()) {
        capture->unlink();
        return true;
    }

    auto imageData = capture->pendingImages.dequeue();
    if (imageData.filename.isEmpty()) {
        capture->unlink();
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
    }

    capture->unlink();

    return TRUE;
}

void QGstreamerImageCapture::unlink()
{
    return;
    if (passImage)
        return;
    if (gstPipeline.isNull())
        return;
    gstPipeline.beginConfig();
    videoSrcPad.unlinkPeer();
    m_session->releaseVideoPad(videoSrcPad);
    videoSrcPad = {};
    bin.setStateSync(GST_STATE_READY);
    bin.lockState(true);
    gstPipeline.endConfig();
}

void QGstreamerImageCapture::link()
{
    if (!(m_session && m_session->camera()))
        return;
    if (!bin.staticPad("sink").peer().isNull() || gstPipeline.isNull())
        return;
    gstPipeline.beginConfig();
    videoSrcPad = m_session->getVideoPad();
    videoSrcPad.link(bin.staticPad("sink"));
    bin.lockState(false);
    bin.setState(GST_STATE_PAUSED);
    gstPipeline.endConfig();
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
