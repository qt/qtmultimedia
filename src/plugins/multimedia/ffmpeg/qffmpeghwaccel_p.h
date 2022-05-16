/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include <qshareddata.h>
#include <memory>

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
    virtual std::unique_ptr<QRhiTexture> texture(int plane);
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
    struct Data {
        ~Data();
        QAtomicInt ref = 0;
        AVBufferRef *hwDeviceContext = nullptr;
        AVBufferRef *hwFramesContext = nullptr;
    };

public:
    HWAccel() = default;
    explicit HWAccel(AVHWDeviceType deviceType);
    explicit HWAccel(const AVCodec *codec);
    ~HWAccel();

    bool isNull() const { return !d || !d->hwDeviceContext; }

    AVHWDeviceType deviceType() const;

    AVBufferRef *hwDeviceContextAsBuffer() const { return d ? d->hwDeviceContext : nullptr; }
    AVHWDeviceContext *hwDeviceContext() const;
    AVPixelFormat hwFormat() const;

    const AVCodec *hardwareEncoderForCodecId(AVCodecID id) const;
    static HWAccel findHardwareAccelForCodecID(AVCodecID id);

    static const AVCodec *hardwareDecoderForCodecId(AVCodecID id);

    void createFramesContext(AVPixelFormat swFormat, const QSize &size);
    AVBufferRef *hwFramesContextAsBuffer() const { return d ? d->hwFramesContext : nullptr; }
    AVHWFramesContext *hwFramesContext() const;

    static AVPixelFormat format(AVFrame *frame);
    static const AVHWDeviceType *preferredDeviceTypes();
private:
    QExplicitlySharedDataPointer<Data> d;
};

}

QT_END_NAMESPACE

#endif
