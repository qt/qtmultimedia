// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamerimagecapture_p.h"

#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/private/qmediastoragelocation_p.h>
#include <QtMultimedia/private/qplatformcamera_p.h>
#include <QtMultimedia/private/qplatformimagecapture_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>
#include <QtGui/qguiapplication.h>
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

Q_STATIC_LOGGING_CATEGORY(qLcImageCaptureGst, "qt.multimedia.imageCapture")

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

        m_instance = new QThreadPool;
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

template <typename Functor>
void QGstreamerImageCapture::invokeDeferred(Functor &&fn)
{
    QMetaObject::invokeMethod(this, std::forward<decltype(fn)>(fn), Qt::QueuedConnection);
}

template <typename Functor>
void QGstreamerImageCapture::runInThreadPool(Functor fn)
{
    int futureId = m_futureIDAllocator.fetch_add(1, std::memory_order_relaxed);

    QFuture<void> future = QtConcurrent::run([this, futureId, fn = std::move(fn)]() mutable {
        auto cleanup = qScopeGuard([&] {
            QMutexLocker guard(&m_pendingFuturesMutex);
            m_pendingFutures.erase(futureId);
        });
        fn();
    });

    if (!future.isValid()) // during qApplication shutdown the threadpool becomes unusable
        return;

    QMutexLocker guard(&m_pendingFuturesMutex);
    m_pendingFutures.emplace(futureId, std::move(future));
}

q23::expected<QPlatformImageCapture *, QString> QGstreamerImageCapture::create(QImageCapture *parent)
{
    static const auto error = qGstErrorMessageIfElementsNotAvailable(
            "queue", "capsfilter", "videoconvert", "jpegenc", "jifmux", "fakesink");
    if (error)
        return q23::unexpected{ *error };

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
    queue.set("max-size-buffers", int(1));
    queue.set("max-size-bytes", int(0));
    queue.set("max-size-time", uint64_t(0));

    bin.add(queue, filter, videoConvert, encoder, muxer, sink);
    qLinkGstElements(queue, filter, videoConvert, encoder, muxer, sink);
    bin.addGhostPad(queue, "sink");

    addProbeToPad(queue.staticPad("src").pad(), false);

    sink.set("async", false);
}

QGstreamerImageCapture::~QGstreamerImageCapture()
{
    bin.setStateSync(GST_STATE_NULL);

    // wait for pending futures
    auto pendingFutures = [&] {
        QMutexLocker guard(&m_pendingFuturesMutex);
        return std::move(m_pendingFutures);
    }();

    for (auto &element : pendingFutures)
        element.second.waitForFinished();
}

bool QGstreamerImageCapture::isReadyForCapture() const
{
    QMutexLocker guard(&m_mutex);
    return m_session && !m_captureNextBuffer && cameraActive;
}

int QGstreamerImageCapture::capture(const QString &fileName)
{
    using namespace Qt::Literals;
    QString path = QMediaStorageLocation::generateFileName(
            fileName, QStandardPaths::PicturesLocation, u"jpg"_s);
    return doCapture(std::move(path));
}

int QGstreamerImageCapture::captureToBuffer()
{
    return doCapture(QString());
}

int QGstreamerImageCapture::doCapture(QString fileName)
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
        if (m_captureNextBuffer) {
            invokeDeferred([this] {
                emit error(-1, QImageCapture::NotReadyError,
                           QPlatformImageCapture::msgCameraNotReady());
            });

            qCDebug(qLcImageCaptureGst) << "error 3";
            return -1;
        }
        m_lastId++;

        pendingImages.enqueue({ m_lastId, std::move(fileName) });
        // let one image pass the pipeline
        m_captureNextBuffer = true;
    }

    emit readyForCaptureChanged(false);
    return m_lastId;
}

void QGstreamerImageCapture::saveBufferToFile(QGstBufferHandle buffer, QString filename, int taskId)
{
    Q_ASSERT(!filename.isEmpty());

    runInThreadPool(
            [this, taskId, filename = std::move(filename), buffer = std::move(buffer)]() mutable {
        QMutexLocker guard(&m_mutex);
        qCDebug(qLcImageCaptureGst) << "saving image as" << filename;

        QFile f(filename);
        if (!f.open(QFile::WriteOnly)) {
            qCDebug(qLcImageCaptureGst) << "   could not open image file for writing";
            return;
        }

        GstMapInfo info;
        if (gst_buffer_map(buffer.get(), &info, GST_MAP_READ)) {
            f.write(reinterpret_cast<const char *>(info.data), info.size);
            gst_buffer_unmap(buffer.get(), &info);
        }
        f.close();

        QMetaObject::invokeMethod(this, [this, taskId, filename = std::move(filename)]() mutable {
            emit imageSaved(taskId, filename);
        });
    });
}
void QGstreamerImageCapture::convertBufferToImage(const QMutexLocker<QRecursiveMutex> &locker,
                                                  QGstBufferHandle buffer, QGstCaps caps,
                                                  QMediaMetaData metadata, int taskId)
{
    using namespace Qt::Literals;
    Q_ASSERT(locker.mutex() == &m_mutex);
    Q_ASSERT(locker.isLocked());

    // QTBUG-131107: QVideoFrame::toImage() can only be called from the application thread
    constexpr bool isOpenGLPlatform = QT_CONFIG(opengl);

    // QTBUG-130970: QVideoFrame::toImage() on worker thread causes wayland to crash on the
    // application thread
    static const bool isWaylandQPA = QGuiApplication::platformName() == u"wayland"_s;

    if (isOpenGLPlatform || isWaylandQPA) {
        if (!m_session) {
            qDebug() << "QGstreamerImageCapture::convertBufferToImage: no session";
            return;
        }
        auto memoryFormat = caps.memoryFormat();

        GstVideoInfo previewInfo;
        QVideoFrameFormat fmt;
        auto optionalFormatAndVideoInfo = caps.formatAndVideoInfo();
        if (optionalFormatAndVideoInfo)
            std::tie(fmt, previewInfo) = std::move(*optionalFormatAndVideoInfo);

        auto *sink = m_session->gstreamerVideoSink();
        auto gstBuffer = std::make_unique<QGstVideoBuffer>(std::move(buffer), previewInfo, sink,
                                                           fmt, memoryFormat);
        QVideoFrame frame = QVideoFramePrivate::createFrame(std::move(gstBuffer), fmt);

        metadata.insert(QMediaMetaData::Resolution, frame.size());

        invokeDeferred(
                [this, frame = std::move(frame), taskId, metadata = std::move(metadata)]() mutable {
            QImage img = frame.toImage();
            if (img.isNull()) {
                qDebug() << "received a null image";
                return;
            }

            emit imageExposed(taskId);
            qCDebug(qLcImageCaptureGst) << "Image available!";
            emit imageAvailable(taskId, frame);
            emit imageCaptured(taskId, img);
            emit imageMetadataAvailable(taskId, metadata);
        });
    } else {
        runInThreadPool([this, taskId, buffer = std::move(buffer), caps = std::move(caps),
                         metadata = std::move(metadata)]() mutable {
            QMutexLocker guard(&m_mutex);
            if (!m_session) {
                qDebug() << "QGstreamerImageCapture::probeBuffer: no session";
                return;
            }

            auto memoryFormat = caps.memoryFormat();

            GstVideoInfo previewInfo;
            QVideoFrameFormat fmt;
            auto optionalFormatAndVideoInfo = caps.formatAndVideoInfo();
            if (optionalFormatAndVideoInfo)
                std::tie(fmt, previewInfo) = std::move(*optionalFormatAndVideoInfo);

            auto *sink = m_session->gstreamerVideoSink();
            auto gstBuffer = std::make_unique<QGstVideoBuffer>(std::move(buffer), previewInfo, sink,
                                                               fmt, memoryFormat);

            QVideoFrame frame = QVideoFramePrivate::createFrame(std::move(gstBuffer), fmt);
            QImage img = frame.toImage();
            if (img.isNull()) {
                qDebug() << "received a null image";
                return;
            }

            QMediaMetaData imageMetaData = metaData();
            imageMetaData.insert(QMediaMetaData::Resolution, frame.size());

            invokeDeferred([this, taskId, metadata = std::move(metadata), frame = std::move(frame),
                            img = std::move(img)]() mutable {
                emit imageExposed(taskId);
                qCDebug(qLcImageCaptureGst) << "Image available!";
                emit imageAvailable(taskId, frame);
                emit imageCaptured(taskId, img);
                emit imageMetadataAvailable(taskId, metadata);
            });
        });
    }
}

void QGstreamerImageCapture::setResolution(const QSize &resolution)
{
    QGstCaps padCaps = bin.staticPad("sink").currentCaps();
    if (!padCaps) {
        qDebug() << "Camera not ready";
        return;
    }
    QGstCaps caps = padCaps.copy();
    if (!caps)
        return;

    gst_caps_set_simple(caps.caps(), "width", G_TYPE_INT, resolution.width(), "height", G_TYPE_INT,
                        resolution.height(), nullptr);
    filter.set("caps", caps);
}

bool QGstreamerImageCapture::probeBuffer(GstBuffer *buffer)
{
    if (!m_captureNextBuffer.load())
        return false;

    QMutexLocker guard(&m_mutex);
    qCDebug(qLcImageCaptureGst) << "probe buffer";

    QGstBufferHandle bufferHandle{
        buffer,
        QGstBufferHandle::NeedsRef,
    };

    m_captureNextBuffer = false;

    bool ready = isReadyForCapture();
    invokeDeferred([this, ready] {
        emit readyForCaptureChanged(ready);
    });

    // save file
    PendingImage imageData = pendingImages.dequeue();
    QString saveFileName = imageData.filename;
    if (!saveFileName.isEmpty())
        saveBufferToFile(bufferHandle, std::move(saveFileName), imageData.id);

    // convert to image and emit
    QGstCaps caps = bin.staticPad("sink").currentCaps();
    QMediaMetaData imageMetaData = metaData();
    convertBufferToImage(guard, bufferHandle, std::move(caps), std::move(imageMetaData),
                         imageData.id);

    return true;
}

void QGstreamerImageCapture::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QMutexLocker guard(&m_mutex);
    QGstreamerMediaCaptureSession *captureSession = static_cast<QGstreamerMediaCaptureSession *>(session);
    if (m_session == captureSession)
        return;

    bool readyForCapture = isReadyForCapture();
    if (m_session) {
        disconnect(m_session, nullptr, this, nullptr);
        m_lastId = 0;
        pendingImages.clear();
        m_captureNextBuffer = false;
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
