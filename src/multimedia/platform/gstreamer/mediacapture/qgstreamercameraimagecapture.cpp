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

#include "qgstreamercameraimagecapture_p.h"
#include "qplatformcamera_p.h"
#include <private/qgstvideobuffer_p.h>
#include <private/qgstutils_p.h>
#include <qvideosurfaceformat.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <qstandardpaths.h>

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcImageCapture, "qt.multimedia.imageCapture")

QGstreamerCameraImageCapture::QGstreamerCameraImageCapture(QGstreamerMediaCapture *session, const QGstPipeline &pipeline)
  : QPlatformCameraImageCapture(session),
    QGstreamerBufferProbe(ProbeBuffers),
    m_session(session),
    gstPipeline(pipeline)
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
    sink = QGstElement("fakesink","imageCaptureSink");
    bin.add(queue, videoConvert, encoder, sink);
    queue.link(videoConvert, encoder, sink);
    bin.addGhostPad(queue, "sink");
    bin.lockState(true);
    gstPipeline.add(bin);

    addProbeToPad(queue.staticPad("src").pad(), false);

    sink.set("signal-handoffs", true);
    g_signal_connect(sink.object(), "handoff", G_CALLBACK(&QGstreamerCameraImageCapture::saveImageFilter), this);

    connect(m_session->addCamera(), &QPlatformCamera::activeChanged, this, &QGstreamerCameraImageCapture::cameraActiveChanged);
    cameraActive = m_session->addCamera()->isActive();
}

QGstreamerCameraImageCapture::~QGstreamerCameraImageCapture()
{
    m_session->releaseVideoPad(videoSrcPad);
}

bool QGstreamerCameraImageCapture::isReadyForCapture() const
{
    return !passImage && cameraActive;
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


int QGstreamerCameraImageCapture::capture(const QString &fileName)
{
    QString path = fileName;
    if (path.isEmpty())
        path = generateFileName(defaultDir(), QLatin1String("jpg"));

    return doCapture(path);
}

int QGstreamerCameraImageCapture::captureToBuffer()
{
    return doCapture(QString());
}

int QGstreamerCameraImageCapture::doCapture(const QString &fileName)
{
    if (!m_session->addCamera()) {
        //emit error in the next event loop,
        //so application can associate it with returned request id.
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                  Q_ARG(int, m_lastId),
                                  Q_ARG(int, QCameraImageCapture::ResourceError),
                                  Q_ARG(QString,tr("No camera available.")));

        return -1;
    }
    if (passImage) {
        //emit error in the next event loop,
        //so application can associate it with returned request id.
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                  Q_ARG(int, -1),
                                  Q_ARG(int, QCameraImageCapture::NotReadyError),
                                  Q_ARG(QString,tr("Camera is not ready.")));
        return -1;
    }
    m_lastId++;

    pendingImages.enqueue({m_lastId, fileName});
    // let one image pass the pipeline
    passImage = true;

    link();

    m_session->dumpGraph(QLatin1String("captureImage"));

    emit readyForCaptureChanged(false);
    return m_lastId;
}

bool QGstreamerCameraImageCapture::probeBuffer(GstBuffer *buffer)
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
    auto fmt = QGstUtils::formatForCaps(caps, &previewInfo, QVideoFrame::NoHandle);
    QVideoFrame frame(gstBuffer, fmt.frameSize(), fmt.pixelFormat());
    QImage img = frame.image();
    if (img.isNull())
        return true;

    auto &imageData = pendingImages.head();

    static QMetaMethod exposedSignal = QMetaMethod::fromSignal(&QGstreamerCameraImageCapture::imageExposed);
    exposedSignal.invoke(this,
                         Qt::QueuedConnection,
                         Q_ARG(int, imageData.id));

    static QMetaMethod capturedSignal = QMetaMethod::fromSignal(&QGstreamerCameraImageCapture::imageCaptured);
    capturedSignal.invoke(this,
                          Qt::QueuedConnection,
                          Q_ARG(int, imageData.id),
                          Q_ARG(QImage, img));

    // #### metadata missing

    return true;
}

void QGstreamerCameraImageCapture::cameraActiveChanged(bool active)
{
    qCDebug(qLcImageCapture) << "cameraActiveChanged" << cameraActive << active;
    if (cameraActive == active)
        return;
    cameraActive = active;
    qCDebug(qLcImageCapture) << "isReady" << isReadyForCapture();
    emit readyForCaptureChanged(isReadyForCapture());
}

gboolean QGstreamerCameraImageCapture::saveImageFilter(GstElement *element,
                                                       GstBuffer *buffer,
                                                       GstPad *pad,
                                                       void *appdata)
{
    Q_UNUSED(element);
    Q_UNUSED(pad);
    QGstreamerCameraImageCapture *capture = static_cast<QGstreamerCameraImageCapture *>(appdata);

    if (capture->pendingImages.isEmpty())
        return true;

    auto imageData = capture->pendingImages.dequeue();

    qCDebug(qLcImageCapture) << "saving image as" << imageData.filename;

    if (!imageData.filename.isEmpty()) {
        QFile f(imageData.filename);
        if (f.open(QFile::WriteOnly)) {
            GstMapInfo info;
            if (gst_buffer_map(buffer, &info, GST_MAP_READ)) {
                f.write(reinterpret_cast<const char *>(info.data), info.size);
                gst_buffer_unmap(buffer, &info);
            }
            f.close();

            static QMetaMethod savedSignal = QMetaMethod::fromSignal(&QGstreamerCameraImageCapture::imageSaved);
            savedSignal.invoke(capture,
                               Qt::QueuedConnection,
                               Q_ARG(int, imageData.id),
                               Q_ARG(QString, imageData.filename));
        }
    } else {
        // ### expose buffer as video frame
    }

    capture->unlink();

    return TRUE;
}

void QGstreamerCameraImageCapture::unlink()
{
    return;
    if (passImage)
        return;

    gstPipeline.setStateSync(GST_STATE_PAUSED);
    videoSrcPad.unlinkPeer();
    m_session->releaseVideoPad(videoSrcPad);
    videoSrcPad = {};
    bin.setStateSync(GST_STATE_READY);
    bin.lockState(true);
    gstPipeline.setStateSync(GST_STATE_PLAYING);
}

void QGstreamerCameraImageCapture::link()
{
    Q_ASSERT(m_session->addCamera());

    if (!bin.staticPad("sink").peer().isNull())
        return;

    gstPipeline.setStateSync(GST_STATE_PAUSED);
    videoSrcPad = m_session->getVideoPad();
    videoSrcPad.link(bin.staticPad("sink"));
    bin.lockState(false);
    bin.setStateSync(GST_STATE_PAUSED);
    gstPipeline.setStateSync(GST_STATE_PLAYING);
}

QImageEncoderSettings QGstreamerCameraImageCapture::imageSettings() const
{
    return m_settings;
}

void QGstreamerCameraImageCapture::setImageSettings(const QImageEncoderSettings &settings)
{
    if (m_settings != settings) {
        m_settings = settings;
        // ###
    }
}

QT_END_NAMESPACE
