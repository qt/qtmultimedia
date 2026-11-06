// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDIMAGEREGISTRY_P_H
#define QANDROIDIMAGEREGISTRY_P_H

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

#include <QtMultimedia/private/qcaptureslotregistry_p.h>
#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideoframeformat.h>

#include <QtCore/qjniobject.h>
#include <QtCore/qsize.h>
#include <QtCore/qtconfigmacros.h>

#include <media/NdkImageReader.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

struct AImage;
struct AImageReader;

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

inline constexpr std::size_t kAndroidImageCount = 12;
inline constexpr std::size_t kAndroidMinFreeImages = 2;

struct AImageDeleter
{
    void operator()(AImage *image) const;
};

using AImagePtr = std::unique_ptr<AImage, AImageDeleter>;

struct AImageReaderDeleter
{
    void operator()(AImageReader *reader) const;
};

using AImageReaderPtr = std::unique_ptr<AImageReader, AImageReaderDeleter>;

class QAndroidImageReader final : public std::enable_shared_from_this<QAndroidImageReader>
{
public:
    using ImageSink = std::function<void(AImagePtr image, std::shared_ptr<QAndroidImageReader>)>;

    static std::shared_ptr<QAndroidImageReader> create(QSize size, std::uint64_t usage);
    ~QAndroidImageReader();

    QAndroidImageReader(const QAndroidImageReader &) = delete;
    QAndroidImageReader &operator=(const QAndroidImageReader &) = delete;

    QJniObject surface() const { return m_surface; }

    void setSink(ImageSink sink);

    QSize size() const { return m_size; }

private:
    QAndroidImageReader(AImageReaderPtr reader, QJniObject surface, QSize size, uintptr_t id);

    static void onImageAvailable(void *context, AImageReader *reader);
    void deliverImages();

    const AImageReaderPtr m_reader;
    const QJniObject m_surface;
    const QSize m_size;
    const uintptr_t m_id;
    AImageReader_ImageListener m_listener;

#ifdef __cpp_lib_atomic_shared_ptr
    std::atomic<std::shared_ptr<const ImageSink>> m_sink;
#else
    std::shared_ptr<const ImageSink> m_sink;
#endif
};

class QAndroidImageSlot
{
public:
    QAndroidImageSlot() = default;
    QAndroidImageSlot(std::shared_ptr<QAndroidImageReader> reader, AImagePtr image)
        : m_reader(std::move(reader)), m_image(std::move(image))
    {
    }

    QAndroidImageSlot(const QAndroidImageSlot &) = delete;
    QAndroidImageSlot &operator=(const QAndroidImageSlot &) = delete;
    QAndroidImageSlot(QAndroidImageSlot &&) noexcept = default;
    QAndroidImageSlot &operator=(QAndroidImageSlot &&other) noexcept;
    ~QAndroidImageSlot() = default;

    AImage *image() const { return m_image.get(); }

private:
    void reset();

    std::shared_ptr<QAndroidImageReader> m_reader;
    AImagePtr m_image;
};

struct QAndroidImageSource
{
    AImage *image = nullptr; // owned by QAndroidImageSlot
    QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;

    // MaterializableCaptureSource
    std::optional<QAbstractVideoBuffer::MapData> mapReadOnly() const;
    void unmap(QAbstractVideoBuffer::MapData &) const { }
};

class QAndroidImageRegistry final
    : public QtMultimediaPrivate::CaptureSlotRegistry<QAndroidImageRegistry, QAndroidImageSlot,
                                                      kAndroidImageCount>
{
public:
    static std::shared_ptr<QAndroidImageRegistry> create();

    void recycleSlot([[maybe_unused]] QAndroidImageSlot slot) const {}

    // Sets the start time of the frame to the end of the previous one, and its end time to the
    // given timestamp.
    void setFrameTimes(QVideoFrame &frame, std::chrono::microseconds timestamp);

private:
    QAndroidImageRegistry();

    std::optional<std::chrono::microseconds> m_lastTimestamp;
};

QVideoFrame createVideoFrame(QAndroidImageRegistry &registry, QAndroidImageSlot slot,
                             QVideoFrameFormat format);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QANDROIDIMAGEREGISTRY_P_H
