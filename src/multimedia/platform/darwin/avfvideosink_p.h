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

#ifndef AVFVIDEOWINDOWCONTROL_H
#define AVFVIDEOWINDOWCONTROL_H

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

#include "private/qplatformvideosink_p.h"

Q_FORWARD_DECLARE_OBJC_CLASS(CALayer);
Q_FORWARD_DECLARE_OBJC_CLASS(AVPlayerLayer);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureVideoPreviewLayer);

#include <CoreVideo/CVBase.h>
#include <CoreVideo/CVPixelBuffer.h>
#include <CoreVideo/CVImageBuffer.h>

#import "Metal/Metal.h"
#import "MetalKit/MetalKit.h"

QT_BEGIN_NAMESPACE

class AVFVideoSinkInterface;

class AVFVideoSink : public QPlatformVideoSink
{
    Q_OBJECT

public:
    AVFVideoSink(QVideoSink *parent = nullptr);
    virtual ~AVFVideoSink();

    // QPlatformVideoSink interface
public:
    void setRhi(QRhi *rhi) override;

    void setNativeSize(QSize size);

    void setVideoSinkInterface(AVFVideoSinkInterface *interface);

private:
    AVFVideoSinkInterface *m_interface = nullptr;
    QRhi *m_rhi = nullptr;
};

class AVFVideoSinkInterface
{
public:
    ~AVFVideoSinkInterface();

    void setVideoSink(AVFVideoSink *sink);


    virtual void reconfigure() = 0;
    virtual void setRhi(QRhi *);
    virtual void setLayer(CALayer *layer);
    virtual void setOutputSettings(NSDictionary *settings);

    QRhi *rhi() const { return m_rhi; }

    void updateLayerBounds();
    void nativeSizeChanged() { updateLayerBounds(); }
    QSize nativeSize() const { return m_sink ? m_sink->nativeSize() : QSize(); }

    CVMetalTextureCacheRef cvMetalTextureCache = nullptr;
#if defined(Q_OS_MACOS)
    CVOpenGLTextureCacheRef cvOpenGLTextureCache = nullptr;
#elif defined(Q_OS_IOS)
    CVOpenGLESTextureCacheRef cvOpenGLESTextureCache = nullptr;
#endif
private:
    void freeTextureCaches();

protected:

    AVFVideoSink *m_sink = nullptr;
    QRhi *m_rhi = nullptr;
    CALayer *m_layer = nullptr;
    NSDictionary *m_outputSettings = nullptr;
};


QT_END_NAMESPACE

#endif // AVFVIDEOWINDOWCONTROL_H
