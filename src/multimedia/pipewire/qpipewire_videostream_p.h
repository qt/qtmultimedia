// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_VIDEOSTREAM_P_H
#define QPIPEWIRE_VIDEOSTREAM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/private/qdmabufallocator_p.h>
#include <QtMultimedia/private/qpipewire_cameracontextmanager_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmutex.h>
#include <QtCore/qobject.h>
#include <QtCore/qsize.h>

#include <bitset>
#include <chrono>
#include <optional>

#include <spa/buffer/buffer.h>
#include <spa/param/video/format.h>

#include <pipewire/version.h>

// DMABUF negotiation needs SPA_FORMAT_VIDEO_modifier/SPA_VIDEO_FLAG_MODIFIER(_FIXATION_REQUIRED)
// and SPA_PARAM_BUFFERS_dataType, none of which older libspa headers declare.
#if QT_CONFIG(linux_dmabuf) && PW_CHECK_VERSION(0, 3, 76)
#  define QT_PIPEWIRE_HAS_DMABUF_CAMERA 1
#else
#  define QT_PIPEWIRE_HAS_DMABUF_CAMERA 0
#endif

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

using BufferDataTypes = std::bitset<SPA_DATA_DmaBuf + 1>;

#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
class DmaBufBufferRegistry;
#endif

class Q_MULTIMEDIA_EXPORT QPipeWireVideoStream : public QObject
{
    Q_OBJECT

public:
    QPipeWireVideoStream(QCameraContextManager &context, QByteArray targetObject,
                         QVideoFrameFormat::PixelFormat requestedPixelFormat, QSize requestedSize,
                         qreal requestedFrameRate, bool allowStreamDmaBufs);
    ~QPipeWireVideoStream() override;

    Q_DISABLE_COPY_MOVE(QPipeWireVideoStream)

    bool start();

    QVideoFrameFormat frameFormat() const;

Q_SIGNALS:
    void newVideoFrame(const QVideoFrame &frame);
    void errorOccurred(const QString &errorString);

private:
    void onStateChanged(pw_stream_state old, pw_stream_state state, const char *error);
    void onParamChanged(uint32_t id, const spa_pod *param);
    void onProcess();

#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
    // Answers a SPA_VIDEO_FLAG_MODIFIER_FIXATION_REQUIRED request with the one
    // modifier we can read, which makes the server send a fixated format.
    void confirmDmaBufModifier(spa_video_format, spa_rectangle);

    void onAddBuffer(pw_buffer *buffer);
    void allocateDmaBufBuffers(QSize size, QVideoFrameFormat::PixelFormat pixelFormat);
    bool emitDmaBufFrame(pw_buffer *buffer, const QVideoFrameFormat &frameFormat);
#endif

    static const pw_stream_events &streamEvents();

    QCameraContextManager &m_context;
    const QByteArray m_targetObject;
    const QVideoFrameFormat::PixelFormat m_requestedPixelFormat;
    const QSize m_requestedSize;
    const qreal m_requestedFrameRate;
    const bool m_allowStreamDmaBufs;

    PwStreamHandle m_stream;
    spa_hook m_streamListener{};

    mutable QMutex m_formatMutex;
    QVideoFrameFormat m_frameFormat;

    // Only meaningful on the event loop thread. A DMABUF buffer is only read when
    // its layout is known: a compressed byte stream, or raw with a linear modifier.
    bool m_dmaBufReadable = false;
    BufferDataTypes m_bufferDataTypes;

    // The first frame's timestamp in microseconds; frame start times are relative to it.
    std::optional<std::chrono::microseconds> m_firstFrameTime;

#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
    // Set when the stream allocates its DMA-BUF buffers itself, so they outlive the stream.
    std::optional<QtMultimediaPrivate::QDmaBufAllocator> m_dmaBufAllocator;
    std::shared_ptr<DmaBufBufferRegistry> m_bufferRegistry;
#endif
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_VIDEOSTREAM_P_H
