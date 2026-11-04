// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidimageregistry_p.h"

#include <QtMultimedia/private/qmaterializingvideobuffer_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>

#include <QtCore/private/qexpected_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qmutex.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <media/NdkImage.h>
#include <media/NdkImageReader.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcAndroidImageReader, "qt.multimedia.ffmpeg.android.camera");
Q_STATIC_LOGGING_CATEGORY(qLcAndroidImageRegistry, "qt.multimedia.ffmpeg.android.camera.registry");

using namespace Qt::StringLiterals;

namespace QFFmpeg {

namespace {

QMutex g_registryMutex;
std::unordered_map<uintptr_t, std::weak_ptr<QAndroidImageReader>> g_registry;
std::atomic<uintptr_t> g_imageIdAllocator{ 1 };

struct ReaderDeleter
{
    void operator()(QAndroidImageReader *reader) const
    {
        auto capturedReader = std::unique_ptr<QAndroidImageReader>(reader);
        if (!qApp)
            return;

        // ensure delete on app thread
        QMetaObject::invokeMethod(qApp, [reader = std::move(capturedReader)] {
        });
    }
};

} // namespace

void AImageDeleter::operator()(AImage *image) const
{
    AImage_delete(image);
}

void AImageReaderDeleter::operator()(AImageReader *reader) const
{
    AImageReader_delete(reader);
}

std::shared_ptr<QAndroidImageReader> QAndroidImageReader::create(QSize size, std::uint64_t usage)
{
    AImageReader *rawReader = nullptr;
    const media_status_t status =
            AImageReader_newWithUsage(size.width(), size.height(), AIMAGE_FORMAT_YUV_420_888, usage,
                                      int32_t(kAndroidImageCount), &rawReader);

    AImageReaderPtr reader(rawReader);
    if (status != AMEDIA_OK || !reader) {
        qCWarning(qLcAndroidImageReader) << "AImageReader_newWithUsage failed for" << size;
        return nullptr;
    }

    ANativeWindow *window = nullptr;
    if (AImageReader_getWindow(reader.get(), &window) != AMEDIA_OK || !window) {
        qCWarning(qLcAndroidImageReader) << "AImageReader_getWindow failed";
        return nullptr;
    }

    QJniEnvironment env;
    jobject rawSurface = ANativeWindow_toSurface(env.jniEnv(), window);
    if (!rawSurface) {
        qCWarning(qLcAndroidImageReader) << "ANativeWindow_toSurface failed";
        return nullptr;
    }

    QJniObject surface(rawSurface);
    env.jniEnv()->DeleteLocalRef(rawSurface);

    const uintptr_t id = g_imageIdAllocator.fetch_add(1, std::memory_order_relaxed);
    std::shared_ptr<QAndroidImageReader> holder{
        new QAndroidImageReader(std::move(reader), std::move(surface), size, id),
        ReaderDeleter{}
    };

    {
        std::lock_guard lock(g_registryMutex);
        g_registry.emplace(id, holder);
    }

    if (AImageReader_setImageListener(holder->m_reader.get(), &holder->m_listener) != AMEDIA_OK) {
        qCWarning(qLcAndroidImageReader) << "AImageReader_setImageListener failed";
        std::lock_guard lock(g_registryMutex);
        g_registry.erase(id);
        return nullptr;
    }

    return holder;
}

QAndroidImageReader::QAndroidImageReader(AImageReaderPtr reader, QJniObject surface, QSize size,
                                         uintptr_t id)
    : m_reader(std::move(reader)),
      m_surface(std::move(surface)),
      m_size(size),
      m_id(id),
      m_listener{
          reinterpret_cast<void *>(id),
          &onImageAvailable,
      }
{
}

QAndroidImageReader::~QAndroidImageReader()
{
    {
        std::lock_guard lock(g_registryMutex);
        g_registry.erase(m_id);
    }

    AImageReader_setImageListener(m_reader.get(), nullptr);
}

void QAndroidImageReader::setSink(ImageSink sink)
{
    std::shared_ptr<const ImageSink> storedSink;
    if (sink)
        storedSink = std::make_shared<const ImageSink>(std::move(sink));

#ifdef __cpp_lib_atomic_shared_ptr
    m_sink.store(std::move(storedSink), std::memory_order_release);
#else
    std::atomic_store_explicit(&m_sink, std::move(storedSink), std::memory_order_release);
#endif
}

void QAndroidImageReader::onImageAvailable(void *context, AImageReader *)
{
    const auto id = reinterpret_cast<uintptr_t>(context);

    std::shared_ptr<QAndroidImageReader> self;
    {
        std::lock_guard lock(g_registryMutex);
        if (auto it = g_registry.find(id); it != g_registry.end())
            self = it->second.lock();
    }

    if (!self)
        // The reader has been destroyed asyncronously
        return;

    self->deliverImages();
}

void QAndroidImageReader::deliverImages()
{
    std::shared_ptr<const ImageSink> sink;
#ifdef __cpp_lib_atomic_shared_ptr
    sink = m_sink.load(std::memory_order_acquire);
#else
    sink = std::atomic_load_explicit(&m_sink, std::memory_order_acquire);
#endif
    if (!sink)
        return;

    AImage *rawImage = nullptr;
    const media_status_t status = AImageReader_acquireLatestImage(m_reader.get(), &rawImage);
    switch (status) {
    case AMEDIA_OK: {
        (*sink)(AImagePtr(rawImage), shared_from_this());
        return;
    }
    case AMEDIA_IMGREADER_NO_BUFFER_AVAILABLE:
        return;
    default:
        qCWarning(qLcAndroidImageReader)
                << "AImageReader_acquireLatestImage failed, status" << status;
        return;
    }
}

std::optional<QAbstractVideoBuffer::MapData> QAndroidImageSource::mapReadOnly() const
{
    if (!image)
        return std::nullopt;

    const bool planar = pixelFormat == QVideoFrameFormat::Format_YUV420P;
    // Android reports the size of each chroma plane as the smallest span covering its samples.
    // The interleaved UV plane is one byte longer, and the first chroma plane in memory is the
    // one that starts it.
    const int firstUvPlane = pixelFormat == QVideoFrameFormat::Format_NV21 ? 2 : 1;

    QAbstractVideoBuffer::MapData mapData;
    mapData.planeCount = planar ? 3 : 2;
    for (int i = 0; i < mapData.planeCount; ++i) {
        const int imagePlane = !planar && i == 1 ? firstUvPlane : i;
        uint8_t *data = nullptr;
        int32_t length = 0;
        int32_t rowStride = 0;
        if (AImage_getPlaneData(image, imagePlane, &data, &length) != AMEDIA_OK
            || AImage_getPlaneRowStride(image, imagePlane, &rowStride) != AMEDIA_OK) {
            qCWarning(qLcAndroidImageRegistry) << "Cannot read plane" << imagePlane;
            return std::nullopt;
        }
        mapData.data[i] = data;
        mapData.bytesPerLine[i] = rowStride;
        mapData.dataSize[i] = length;
    }

    if (!planar)
        ++mapData.dataSize[1];

    return mapData;
}

QAndroidImageSlot &QAndroidImageSlot::operator=(QAndroidImageSlot &&other) noexcept
{
    if (this != &other) {
        reset();
        m_reader = std::move(other.m_reader);
        m_image = std::exchange(other.m_image, nullptr);
    }
    return *this;
}

void QAndroidImageSlot::reset()
{
    m_image.reset();
    m_reader.reset();
}

std::shared_ptr<QAndroidImageRegistry> QAndroidImageRegistry::create()
{
    return std::shared_ptr<QAndroidImageRegistry>(new QAndroidImageRegistry());
}

QAndroidImageRegistry::QAndroidImageRegistry()
    : CaptureSlotRegistry(kAndroidImageCount, kAndroidMinFreeImages)
{
}

using ImageVideoBuffer =
        QtMultimediaPrivate::MaterializingVideoBuffer<QAndroidImageSource, QAndroidImageRegistry>;

namespace {

// Derives the pixel format from the chroma planes alone. The rest of the image is read when the
// frame is mapped.
q23::expected<QVideoFrameFormat::PixelFormat, QString> getPixelFormat(AImage *image)
{
    int32_t planeCount = 0;
    if (AImage_getNumberOfPlanes(image, &planeCount) != AMEDIA_OK || planeCount != 3)
        return q23::unexpected{ u"Unexpected YUV_420_888 plane count %1"_s.arg(planeCount) };

    uint8_t *uData = nullptr;
    uint8_t *vData = nullptr;
    int32_t uLength = 0;
    int32_t vLength = 0;
    int32_t uPixelStride = 0;
    if (AImage_getPlaneData(image, 1, &uData, &uLength) != AMEDIA_OK
        || AImage_getPlaneData(image, 2, &vData, &vLength) != AMEDIA_OK
        || AImage_getPlanePixelStride(image, 1, &uPixelStride) != AMEDIA_OK) {
        return q23::unexpected{ u"Cannot read chroma planes"_s };
    }

    if (uPixelStride == 1)
        return QVideoFrameFormat::Format_YUV420P;

    if (uPixelStride == 2) {
        const std::ptrdiff_t uvOffset = uData - vData;
        if (uvOffset == -1) // interleaved UVUV
            return QVideoFrameFormat::Format_NV12;
        if (uvOffset == 1) // interleaved VUVU
            return QVideoFrameFormat::Format_NV21;
    }

    return q23::unexpected{ u"Unsupported YUV_420_888 layout"_s };
}

q23::expected<QSize, QString> getSize(AImage *image)
{
    int32_t width = 0;
    int32_t height = 0;
    if (AImage_getWidth(image, &width) != AMEDIA_OK
        || AImage_getHeight(image, &height) != AMEDIA_OK) {
        return q23::unexpected{ u"Cannot read image size"_s };
    }

    return QSize(width, height);
}

} // namespace

QVideoFrame createVideoFrame(QAndroidImageRegistry &registry, QAndroidImageSlot slot,
                             QVideoFrameFormat format)
{
    AImage *image = slot.image();
    if (!image)
        return {};

    const auto pixelFormat = getPixelFormat(image);
    if (!pixelFormat) {
        qCWarning(qLcAndroidImageRegistry) << pixelFormat.error();
        return {};
    }

    const auto size = getSize(image);
    if (!size) {
        qCWarning(qLcAndroidImageRegistry) << size.error();
        return {};
    }

    QAndroidImageSource source{
        image,
        *pixelFormat,
    };

    QVideoFrameFormat frameFormat(*size, *pixelFormat);
    frameFormat.setRotation(format.rotation());

    std::unique_ptr<QAbstractVideoBuffer> videoBuffer = ImageVideoBuffer::wrapOrMaterialize(
            registry, std::move(slot), std::move(source), frameFormat);
    return QVideoFramePrivate::createFrame(std::move(videoBuffer), std::move(frameFormat));
}

} // namespace QFFmpeg

QT_END_NAMESPACE
