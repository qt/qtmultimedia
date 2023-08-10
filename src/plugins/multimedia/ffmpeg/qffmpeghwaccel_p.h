// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGHWACCEL_P_H
#define QFFMPEGHWACCEL_P_H

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

#include "qffmpeg_p.h"
#include "qvideoframeformat.h"
#include <private/qabstractvideobuffer_p.h>
#include <qshareddata.h>
#include <memory>
#include <functional>
#include <mutex>

QT_BEGIN_NAMESPACE

class QRhi;
class QRhiTexture;
class QFFmpegVideoBuffer;

namespace QFFmpeg {

// used for the get_format callback for the decoder
enum AVPixelFormat getFormat(struct AVCodecContext *s, const enum AVPixelFormat * fmt);

class HWAccel;

class TextureSet {
public:
    // ### Should add QVideoFrameFormat::PixelFormat here
    virtual ~TextureSet() {}
    virtual qint64 textureHandle(int /*plane*/) { return 0; }
};

class TextureConverterBackend
{
public:
    TextureConverterBackend(QRhi *rhi)
        : rhi(rhi)
    {}
    virtual ~TextureConverterBackend() {}
    virtual TextureSet *getTextures(AVFrame * /*frame*/) { return nullptr; }

    QRhi *rhi = nullptr;
};

class TextureConverter
{
    class Data final
    {
    public:
        ~Data();
        QAtomicInt ref = 0;
        QRhi *rhi = nullptr;
        AVPixelFormat format = AV_PIX_FMT_NONE;
        TextureConverterBackend *backend = nullptr;
    };
public:
    TextureConverter(QRhi *rhi = nullptr);

    void init(AVFrame *frame) {
        AVPixelFormat fmt = frame ? AVPixelFormat(frame->format) : AV_PIX_FMT_NONE;
        if (fmt != d->format)
            updateBackend(fmt);
    }
    TextureSet *getTextures(AVFrame *frame);
    bool isNull() const { return !d->backend || !d->backend->rhi; }

private:
    void updateBackend(AVPixelFormat format);

    QExplicitlySharedDataPointer<Data> d;
};

class HWAccel
{
    AVBufferUPtr m_hwDeviceContext;
    AVBufferUPtr m_hwFramesContext;

    mutable std::once_flag m_constraintsOnceFlag;
    mutable AVHWFramesConstraintsUPtr m_constraints;

public:
    ~HWAccel();

    static std::unique_ptr<HWAccel> create(AVHWDeviceType deviceType);

    static std::pair<const AVCodec *, std::unique_ptr<HWAccel>>
    findEncoderWithHwAccel(AVCodecID id,
                           const std::function<bool(const HWAccel &)>& hwAccelPredicate = nullptr);

    static std::pair<const AVCodec *, std::unique_ptr<HWAccel>>
    findDecoderWithHwAccel(AVCodecID id,
                           const std::function<bool(const HWAccel &)>& hwAccelPredicate = nullptr);

    AVHWDeviceType deviceType() const;

    AVBufferRef *hwDeviceContextAsBuffer() const { return m_hwDeviceContext.get(); }
    AVHWDeviceContext *hwDeviceContext() const;
    AVPixelFormat hwFormat() const;
    const AVHWFramesConstraints *constraints() const;

    void createFramesContext(AVPixelFormat swFormat, const QSize &size);
    AVBufferRef *hwFramesContextAsBuffer() const { return m_hwFramesContext.get(); }
    AVHWFramesContext *hwFramesContext() const;

    static AVPixelFormat format(AVFrame *frame);
    static const std::vector<AVHWDeviceType> &encodingDeviceTypes();

    static const std::vector<AVHWDeviceType> &decodingDeviceTypes();

private:
    HWAccel(AVBufferUPtr hwDeviceContext) : m_hwDeviceContext(std::move(hwDeviceContext)) { }
};

}

QT_END_NAMESPACE

#endif
