// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGTEXTURECONVERTER_P_H
#define QFFMPEGTEXTURECONVERTER_P_H

#include <QtFFmpegMediaPluginImpl/private/qffmpegdefs_p.h>
#include <QtMultimedia/private/qhwvideobuffer_p.h>

#include <memory>

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

QT_BEGIN_NAMESPACE

class QRhi;

namespace QFFmpeg {

/**
 * @brief The base class for platform-specific implementations of TextureConverter
 *        One of two virtual methods, createTextures and createTextureHandles, must be overridden.
 *        If the implementation operates with QRhiTexture under the hood, overriding of
 *        createTextures is prefferable, otherwise expose texture handles of native textures
 *        by createTextureHandles.
 */
class TextureConverterBackend : public std::enable_shared_from_this<TextureConverterBackend>
{
public:
    TextureConverterBackend(QRhi *rhi) : rhi(rhi) { }

    virtual ~TextureConverterBackend();

    virtual QVideoFrameTexturesUPtr createTextures(AVFrame * /*hwFrame*/,
                                                   QVideoFrameTexturesUPtr & /*oldTextures*/)
    {
        return nullptr;
    }

    virtual QVideoFrameTexturesHandlesUPtr
    createTextureHandles(AVFrame * /*hwFrame*/, QVideoFrameTexturesHandlesUPtr /*oldHandles*/)
    {
        return nullptr;
    }

    /**
     * @brief Points to the matching QRhi.
     *        If the constructor, createTextures, or createTextureHandles get failed without
     *        chances for recovery, it may set the pointer to nullptr, which will invalidate
     *        the parent TextureConverter, and textures creation won't be invoked anymore.
     */
    QRhi *rhi = nullptr;
};
using TextureConverterBackendPtr = std::shared_ptr<TextureConverterBackend>;

/**
 * @brief The TextureConverter class implements conversion of AVFrame hw textures to
 *        textures for rendering by the specified QRhi. Any instance of TextureConverter
 *        matches the pair of FFmpeg hw frames context + QRhi.
 */
class TextureConverter
{
public:
    /**
     * @brief Construct uninitialized texture converter for the specified QRhi
     */
    TextureConverter(QRhi &rhi);

    /**
     * @brief Initializes the instance of the texture converter for the frame context
     *        associated with the specified frame. The method tries to initialize
     *        the conversion backend during the first call with the specified frame format.
     *        If frame format is not changed, the method does nothing even if the first
     *        attempt failed.
     * @return Whether the instance has been initialized.
     */
    bool init(AVFrame &hwFrame);

    /**
     * @brief Creates video frame textures basing on the current hw frame and the previous textures
     *        from the texture pool. We should strive to reuse oldTextures if we can do so.
     *        If the method returns null, try createTextureHandles.
     */
    QVideoFrameTexturesUPtr createTextures(AVFrame &hwFrame, QVideoFrameTexturesUPtr &oldTextures);

    /**
     * @brief Creates video frame texture handles basing on the current hw frame and the previous
     *        texture handles from the pool. We should strive to reuse oldHandles if we can do so.
     */
    QVideoFrameTexturesHandlesUPtr createTextureHandles(AVFrame &hwFrame,
                                                        QVideoFrameTexturesHandlesUPtr oldHandles);

    /**
     * @brief Indicates whether the texture converter is not initialized or the initialization
     * failed. If hw texture conversion is disabled, it always true.
     */
    bool isNull() const { return !m_backend || !m_backend->rhi; }

    /**
     * @brief Applies platform-specific hw texture conversion presets for a decoder.
     *        The function is supposed to be invoked for the get_format callback.
     */
    static void applyDecoderPreset(AVPixelFormat format, AVCodecContext &codecContext);

    /**
     * @brief Indicates whether hw texture conversion is enabled for the application.
     */
    static bool hwTextureConversionEnabled();

    /**
     * @brief Indicates whether the matching textute converter backend can be created.
     *        If isBackendAvailable returns false, instances cannot be initialized with
     *        the specified frame. If it returns true, init will attempt to create backend,
     *        but it may fail if something goes wrong in the backend.
     */
    static bool isBackendAvailable(AVFrame &hwFrame);

private:
    void updateBackend(AVPixelFormat format);

private:
    QRhi &m_rhi;
    AVPixelFormat m_format = AV_PIX_FMT_NONE;
    TextureConverterBackendPtr m_backend;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGTEXTURECONVERTER_P_H
