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
#if defined(Q_OS_OSX)
Q_FORWARD_DECLARE_OBJC_CLASS(NSView);
typedef NSView NativeView;
#else
Q_FORWARD_DECLARE_OBJC_CLASS(UIView);
typedef UIView NativeView;
#endif

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
    QVideoSink::GraphicsType graphicsType() const override { return m_graphicsType; }
    bool setGraphicsType(QVideoSink::GraphicsType type) override;

    WId winId() const override;
    void setWinId(WId id) override;

    void setRhi(QRhi *rhi) override;

    QRect displayRect() const override;
    void setDisplayRect(const QRect &rect) override;

    bool isFullScreen() const override;
    void setFullScreen(bool fullScreen) override;

    QSize nativeSize() const override;
    void setNativeSize(QSize size);

    Qt::AspectRatioMode aspectRatioMode() const override;
    void setAspectRatioMode(Qt::AspectRatioMode mode) override;

    int brightness() const override;
    void setBrightness(int brightness) override;

    int contrast() const override;
    void setContrast(int contrast) override;

    int hue() const override;
    void setHue(int hue) override;

    int saturation() const override;
    void setSaturation(int saturation) override;

    void setLayer(CALayer *playerLayer);

    void setVideoSinkInterface(AVFVideoSinkInterface *interface);
    NativeView *nativeView() const { return m_nativeView; }

private:
    AVFVideoSinkInterface *m_interface = nullptr;
    QVideoSink::GraphicsType m_graphicsType = QVideoSink::Memory;
    WId m_winId = 0;
    QRhi *m_rhi = nullptr;
    NativeView *m_nativeView = nullptr;

    QSize m_nativeSize;
    QRect m_displayRect;
    bool m_fullscreen = false;
    int m_brightness = 0;
    int m_contrast = 0;
    int m_hue = 0;
    int m_saturation = 0;
    Qt::AspectRatioMode m_aspectRatioMode = Qt::KeepAspectRatio;
};

class AVFVideoSinkInterface
{
public:
    ~AVFVideoSinkInterface();

    void setVideoSink(AVFVideoSink *sink);


    virtual void reconfigure() = 0;
    virtual void updateAspectRatio() = 0;
    virtual void setRhi(QRhi *) { Q_ASSERT(false); }

    void setLayer(CALayer *layer);

    void renderToNativeView(bool enable);

    bool shouldRenderToWindow()
    {
        return m_layer && nativeView() && (graphicsType() == QVideoSink::NativeWindow || isFullScreen());
    }
    bool rendersToWindow() const { return m_rendersToWindow; }

    void updateLayerBounds();
    void nativeSizeChanged() { updateLayerBounds(); }

protected:
    NativeView *nativeView() const { return m_sink->nativeView(); }
    QRect displayRect() { return m_sink->displayRect(); }
    Qt::AspectRatioMode aspectRatioMode() const { return m_sink->aspectRatioMode(); }
    QVideoSink::GraphicsType graphicsType() const { return m_sink->graphicsType(); }
    bool isFullScreen() const { return m_sink->isFullScreen(); }
    QSize nativeSize() const { return m_sink->nativeSize(); }

    AVFVideoSink *m_sink = nullptr;
    CALayer *m_layer = nullptr;
    bool m_rendersToWindow = false;
};


QT_END_NAMESPACE

#endif // AVFVIDEOWINDOWCONTROL_H
