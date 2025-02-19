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

#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegtextureconverter_p.h>
#include "qvideoframeformat.h"
#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include <QtMultimedia/private/qrhivaluemapper_p.h>

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
AVPixelFormat getFormat(AVCodecContext *s, const AVPixelFormat *fmt);

class HWAccel;

class HWAccel;
using HWAccelUPtr = std::unique_ptr<HWAccel>;

/**
 * @brief The HwFrameContextData class contains custom belongings
 *        of hw frames context.
 */
struct HwFrameContextData
{
    QRhiValueMapper<TextureConverter> textureConverterMapper;

    /**
     * @brief gets or creates an instance of the class, associated with
     *        the frames context of the specified frame. Note, AVFrame
     *        holds shared ownership of the frames context, so consider this
     *        when designing HwFrameContextData's lifetime.
     */
    static HwFrameContextData &ensure(AVFrame &hwFrame);
};

class HWAccel
{
    AVBufferUPtr m_hwDeviceContext;
    AVBufferUPtr m_hwFramesContext;

    mutable std::once_flag m_constraintsOnceFlag;
    mutable AVHWFramesConstraintsUPtr m_constraints;

public:
    ~HWAccel();

    static HWAccelUPtr create(AVHWDeviceType deviceType);

    static std::pair<std::optional<Codec>, HWAccelUPtr> findDecoderWithHwAccel(AVCodecID id);

    AVHWDeviceType deviceType() const;

    AVBufferRef *hwDeviceContextAsBuffer() const { return m_hwDeviceContext.get(); }
    AVHWDeviceContext *hwDeviceContext() const;
    AVPixelFormat hwFormat() const;
    const AVHWFramesConstraints *constraints() const;

    bool matchesSizeContraints(QSize size) const;

    void createFramesContext(AVPixelFormat swFormat, const QSize &size);
    AVBufferRef *hwFramesContextAsBuffer() const { return m_hwFramesContext.get(); }
    AVHWFramesContext *hwFramesContext() const;

    static AVPixelFormat format(AVFrame *frame);
    static const std::vector<AVHWDeviceType> &encodingDeviceTypes();

    static const std::vector<AVHWDeviceType> &decodingDeviceTypes();

private:
    HWAccel(AVBufferUPtr hwDeviceContext) : m_hwDeviceContext(std::move(hwDeviceContext)) { }
};

AVFrameUPtr copyFromHwPool(AVFrameUPtr frame);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif
