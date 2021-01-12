/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef AVFCAMERAWINDOWCONTROL_H
#define AVFCAMERAWINDOWCONTROL_H

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

#include <QVideoWindowControl>

@class AVCaptureVideoPreviewLayer;
#if defined(Q_OS_MACOS)
@class NSView;
typedef NSView NativeView;
#else
@class UIView;
typedef UIView NativeView;
#endif

QT_BEGIN_NAMESPACE

class AVFCameraWindowControl : public QVideoWindowControl
{
    Q_OBJECT
public:
    AVFCameraWindowControl(QObject *parent = nullptr);
    virtual ~AVFCameraWindowControl() override;

    // QVideoWindowControl interface
public:
    WId winId() const override;
    void setWinId(WId id) override;

    QRect displayRect() const override;
    void setDisplayRect(const QRect &rect) override;

    bool isFullScreen() const override;
    void setFullScreen(bool fullScreen) override;

    void repaint() override;

    QSize nativeSize() const override;

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

    // AVF Camera implementation details
    void setNativeSize(QSize size);
    void setLayer(AVCaptureVideoPreviewLayer *capturePreviewLayer);

private:
    void updateAspectRatio();
    void updateCaptureLayerBounds();

    void retainNativeLayer();
    void releaseNativeLayer();

    void attachNativeLayer();
    void detachNativeLayer();

    WId m_winId{0};
    QRect m_displayRect;
    bool m_fullscreen{false};
    Qt::AspectRatioMode m_aspectRatioMode{Qt::IgnoreAspectRatio};
    QSize m_nativeSize;
    AVCaptureVideoPreviewLayer *m_captureLayer{nullptr};
    NativeView *m_nativeView{nullptr};
};

QT_END_NAMESPACE

#endif // AVFCAMERAWINDOWCONTROL_H
