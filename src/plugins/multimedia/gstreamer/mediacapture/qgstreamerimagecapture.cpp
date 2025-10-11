// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamerimagecapture_p.h"

#include <QtMultimedia/private/qplatformcamera_p.h>
#include <QtMultimedia/private/qplatformimagecapture_p.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/private/qmediastoragelocation_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qloggingcategory.h>

#include <common/qgstreamermetadata_p.h>
#include <common/qgstvideobuffer_p.h>
#include <common/qgstutils_p.h>

#include <utility>

QT_BEGIN_NAMESPACE

namespace {
Q_LOGGING_CATEGORY(qLcImageCaptureGst, "qt.multimedia.imageCapture")

struct ThreadPoolSingleton
{
    QObject m_context;
    QMutex m_poolMutex;
    QThreadPool *m_instance{};
    bool m_appUnderDestruction = false;

    QThreadPool *get(const QMutexLocker<QMutex> &)
    {
        if (m_instance)
            return m_instance;
        if (m_appUnderDestruction || !qApp)
            return nullptr;

        using namespace std::chrono;

        m_instance = new QThreadPool(qApp);
        m_instance->setMaxThreadCount(1); // 1 thread;
        static constexpr auto expiryTimeout = minutes(5);
        m_instance->setExpiryTimeout(round<milliseconds>(expiryTimeout).count());

        QObject::connect(qApp, &QCoreApplication::aboutToQuit, &m_context, [&] {
            // we need to make sure that thread-local QRhi is destroyed before the application to
            // prevent QTBUG-124189
            QMutexLocker guard(&m_poolMutex);
            delete m_instance;
            m_instance = {};
            m_appUnderDestruction = true;
        });

        QObject::connect(qApp, &QCoreApplication::destroyed, &m_context, [&] {
            m_appUnderDestruction = false;
        });
        return m_instance;
    }

    template <typename Functor>
    QFuture<void> run(Functor &&f)
    {
        QMutexLocker guard(&m_poolMutex);
        QThreadPool *pool = get(guard);
        if (!pool)
            return QFuture<void>{};

        return QtConcurrent::run(pool, std::forward<Functor>(f));
    }
};

ThreadPoolSingleton s_threadPoolSingleton;

}; // namespace

QMaybe<QPlatformImageCapture *> QGstreamerImageCapture::create(QImageCapture *parent)
{
    static const auto error = qGstErrorMessageIfElementsNotAvailable(
            "queue", "capsfilter", "videoconvert", "jpegenc", "jifmux", "fakesink");
    if (error)
        return *error;

    return new QGstreamerImageCapture(parent);
}

QGstreamerImageCapture::QGstreamerImageCapture(QImageCapture *parent)
    : QPlatformImageCapture(parent),
      QGstreamerBufferProbe(ProbeBuffers),
      bin{
          QGstBin::create("imageCaptureBin"),
      },
      queue{
          QGstElement::createFromFactory("queue", "imageCaptureQueue"),
      },
      filter{
          QGstElement::createFromFactory("capsfilter", "filter"),
      },
      videoConvert{
          QGstElement::createFromFactory("videoconvert", "imageCaptureConvert"),
      },
      encoder{
          QGstElement::createFromFactory("jpegenc", "jpegEncoder"),
      },
      muxer{
          QGstElement::createFromFactory("jifmux", "jpegMuxer"),
      },
      sink{
          QGstElement::createFromFactory("fakesink", "imageCaptureSink"),
      }
{
    // configures the queue to be fast, lightweight and non blocking
    queue.set("leaky", 2 /*downstream*/);
    queue.set("silent", true);
    queue.set("max-size-buffers", uint(1));
    queue.set("max-size-bytes", uint(0));
    queue.set("max-size-time", quint64(0));

    // imageCaptureSink do not wait for a preroll buffer when going READY -> PAUSED
    // as no buffer will arrive until capture() is called
    sink.set("async", false);

    bin.add(queue, filter, videoConvert, encoder, muxer, sink);
    qLinkGstElements(queue, filter, videoConvert, encoder, muxer, sink);
    bin.addGhostPad(queue, "sink");

    addProbeToPad(queue.staticPad("src").pad(), false);

    sink.set("signal-handoffs", true);
    m_handoffConnection = sink.connect("handoff", G_CALLBACK(&saveImageFilter), this);
}

QGstreamerImageCapture::~QGstreamerImageCapture()
{
    bin.setStateSync(GST_STATE_NULL);

    // wait for pending futures
    auto pendingFutures = [&] {
        QMutexLocker guard(&m_mutex);
        return std::move(m_pendingFutures);
    }();

    for (QFuture<void> &pendingImage : pendingFutures)
        pendingImage.waitForFinished();
}

bool QGstreamerImageCapture::isReadyForCapture() const
{
    QMutexLocker guard(&m_mutex);
    return m_session && !passImage && cameraActive;
}

int QGstreamerImageCapture::capture(const QString &fileName)
{
    using namespace Qt::Literals;
    QString path = QMediaStorageLocation::generateFileName(
            fileName, QStandardPaths::PicturesLocation, u"jpg"_s);
    return doCapture(path);
}

int QGstreamerImageCapture::captureToBuffer()
{
    return doCapture(QString());
}

int QGstreamerImageCapture::doCapture(const QString &fileName)
{
    qCDebug(qLcImageCaptureGst) << "do capture";

    {
        QMutexLocker guard(&m_mutex);
        if (!m_session) {
            invokeDeferred([this] {
                emit error(-1, QImageCapture::ResourceError,
                           QPlatformImageCapture::msgImageCaptureNotSet());
            });

            qCDebug(qLcImageCaptureGst) << "error 1";
            return -1;
        }
        if (!m_session->camera()) {
            invokeDeferred([this] {
                emit error(-1, QImageCapture::ResourceError, tr("No camera available."));
            });

            qCDebug(qLcImageCaptureGst) << "error 2";
            return -1;
        }
        if (passImage) {
            invokeDeferred([this] {
                emit error(-1, QImageCapture::NotReadyError,
                           QPlatformImageCapture::msgCameraNotReady());
            });

            qCDebug(qLcImageCaptureGst) << "error 3";
            return -1;
        }
        m_lastId++;

        pendingImages.enqueue({ m_lastId, fileName, QMediaMetaData{} });
        // let one image pass the pipeline
        passImage = true;
    }

    emit readyForCaptureChanged(false);
    return m_lastId;
}

void QGstreamerImageCapture::setResolution(const QSize &resolution)
{
    QGstCaps padCaps = bin.staticPad("sink").currentCaps();
    if (padCaps.isNull()) {
        qDebug() << "Camera not ready";
        return;
    }
    QGstCaps caps = padCaps.copy();
    if (caps.isNull())
        return;

    gst_caps_set_simple(caps.caps(), "width", G_TYPE_INT, resolution.width(), "height", G_TYPE_INT,
                        resolution.height(), nullptr);
    filter.set("caps", caps);
}

// HACK: gcc-10 and earlier reject [=,this] when building with c++17
#if __cplusplus >= 202002L
#  define EQ_THIS_CAPTURE =, this
#else
#  define EQ_THIS_CAPTURE =
#endif

bool QGstreamerImageCapture::probeBuffer(GstBuffer *buffer)
{
    QMutexLocker guard(&m_mutex);

    if (!passImage)
        return false;
    qCDebug(qLcImageCaptureGst) << "probe buffer";

    QGstBufferHandle bufferHandle{
        buffer,
        QGstBufferHandle::NeedsRef,
    };

    passImage = false;

    bool ready = isReadyForCapture();
    invokeDeferred([this, ready] {
        emit readyForCaptureChanged(ready);
    });

    QGstCaps caps = bin.staticPad("sink").currentCaps();
    auto memoryFormat = caps.memoryFormat();

    GstVideoInfo previewInfo;
    QVideoFrameFormat fmt;
    auto optionalFormatAndVideoInfo = caps.formatAndVideoInfo();
    if (optionalFormatAndVideoInfo)
        std::tie(fmt, previewInfo) = std::move(*optionalFormatAndVideoInfo);

    int futureId = futureIDAllocator += 1;

    // ensure QVideoFrame::toImage is executed on a worker thread that is joined before the
    // qApplication is destroyed
    QFuture<void> future = s_threadPoolSingleton.run([EQ_THIS_CAPTURE]() mutable {
        QMutexLocker guard(&m_mutex);
        auto scopeExit = qScopeGuard([&] {
            m_pendingFutures.remove(futureId);
        });

        if (!m_session) {
            qDebug() << "QGstreamerImageCapture::probeBuffer: no session";
            return;
        }

        auto *sink = m_session->gstreamerVideoSink();
        auto *gstBuffer = new QGstVideoBuffer{
            std::move(bufferHandle), previewInfo, sink, fmt, memoryFormat,
        };
        QVideoFrame frame(gstBuffer, fmt);

        QImage img = frame.toImage();
        if (img.isNull()) {
            qDebug() << "received a null image";
            return;
        }

        QMediaMetaData imageMetaData = metaData();
        imageMetaData.insert(QMediaMetaData::Resolution, frame.size());
        pendingImages.head().metaData = std::move(imageMetaData);
        PendingImage pendingImage = pendingImages.head();

        invokeDeferred([this, pendingImage = std::move(pendingImage), frame = std::move(frame),
                        img = std::move(img)]() mutable {
            emit imageExposed(pendingImage.id);
            qCDebug(qLcImageCaptureGst) << "Image available!";
            emit imageAvailable(pendingImage.id, frame);
            emit imageCaptured(pendingImage.id, img);
            emit imageMetadataAvailable(pendingImage.id, pendingImage.metaData);
        });
    });

    if (!future.isValid()) // during qApplication shutdown the threadpool becomes unusable
        return true;

    m_pendingFutures.insert(futureId, future);

    return true;
}

#undef EQ_THIS_CAPTURE

void QGstreamerImageCapture::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QMutexLocker guard(&m_mutex);
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

    connect(m_session, &QPlatformMediaCaptureSession::cameraChanged, this,
            &QGstreamerImageCapture::onCameraChanged);
    onCameraChanged();
}

void QGstreamerImageCapture::setMetaData(const QMediaMetaData &m)
{
    {
        QMutexLocker guard(&m_mutex);
        QPlatformImageCapture::setMetaData(m);
    }

    // ensure taginject injects this metaData
    applyMetaDataToTagSetter(m, muxer);
}

void QGstreamerImageCapture::cameraActiveChanged(bool active)
{
    qCDebug(qLcImageCaptureGst) << "cameraActiveChanged" << cameraActive << active;
    if (cameraActive == active)
        return;
    cameraActive = active;
    qCDebug(qLcImageCaptureGst) << "isReady" << isReadyForCapture();
    emit readyForCaptureChanged(isReadyForCapture());
}

void QGstreamerImageCapture::onCameraChanged()
{
    QMutexLocker guard(&m_mutex);
    if (m_session->camera()) {
        cameraActiveChanged(m_session->camera()->isActive());
        connect(m_session->camera(), &QPlatformCamera::activeChanged, this,
                &QGstreamerImageCapture::cameraActiveChanged);
    } else {
        cameraActiveChanged(false);
    }
}

gboolean QGstreamerImageCapture::saveImageFilter(GstElement *, GstBuffer *buffer, GstPad *,
                                                 QGstreamerImageCapture *capture)
{
    capture->saveBufferToImage(buffer);
    return true;
}

void QGstreamerImageCapture::saveBufferToImage(GstBuffer *buffer)
{
    QMutexLocker guard(&m_mutex);
    passImage = false;

    if (pendingImages.isEmpty())
        return;

    PendingImage imageData = pendingImages.dequeue();
    if (imageData.filename.isEmpty())
        return;

    int id = futureIDAllocator++;
    QGstBufferHandle bufferHandle{
        buffer,
        QGstBufferHandle::NeedsRef,
    };

    QFuture<void> saveImageFuture = QtConcurrent::run([this, imageData, bufferHandle,
                                                       id]() mutable {
        auto cleanup = qScopeGuard([&] {
            QMutexLocker guard(&m_mutex);
            m_pendingFutures.remove(id);
        });

        qCDebug(qLcImageCaptureGst) << "saving image as" << imageData.filename;

        QFile f(imageData.filename);
        if (!f.open(QFile::WriteOnly)) {
            qCDebug(qLcImageCaptureGst) << "   could not open image file for writing";
            return;
        }

        GstMapInfo info;
        GstBuffer *buffer = bufferHandle.get();
        if (gst_buffer_map(buffer, &info, GST_MAP_READ)) {
            f.write(reinterpret_cast<const char *>(info.data), info.size);
            gst_buffer_unmap(buffer, &info);
        }
        f.close();

        QMetaObject::invokeMethod(this, [this, imageData = std::move(imageData)]() mutable {
            emit imageSaved(imageData.id, imageData.filename);
        });
    });

    m_pendingFutures.insert(id, saveImageFuture);
}

QImageEncoderSettings QGstreamerImageCapture::imageSettings() const
{
    return m_settings;
}

void QGstreamerImageCapture::setImageSettings(const QImageEncoderSettings &settings)
{
    if (m_settings != settings) {
        QSize resolution = settings.resolution();
        if (m_settings.resolution() != resolution && !resolution.isEmpty())
            setResolution(resolution);

        m_settings = settings;
    }
}

QT_END_NAMESPACE

#include "moc_qgstreamerimagecapture_p.cpp"
