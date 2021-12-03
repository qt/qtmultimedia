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

#ifndef QVIDEOWINDOW_P_H
#define QVIDEOWINDOW_P_H

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

#include <QWindow>
#include <qtextlayout.h>

#include <QtGui/private/qrhinull_p.h>
#if QT_CONFIG(opengl)
#include <QtGui/private/qrhigles2_p.h>
#include <QOffscreenSurface>
#endif
#if QT_CONFIG(vulkan)
#include <QtGui/private/qrhivulkan_p.h>
#endif
#ifdef Q_OS_WIN
#include <QtGui/private/qrhid3d11_p.h>
#endif
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#include <QtGui/private/qrhimetal_p.h>
#endif

#include <qvideoframe.h>
#include <private/qplatformvideosink_p.h>
#include <private/qvideotexturehelper_p.h>
#include <qbackingstore.h>

QT_BEGIN_NAMESPACE

class QVideoWindow;

class QVideoWindowPrivate {
public:
    QVideoWindowPrivate(QVideoWindow *q);
    ~QVideoWindowPrivate();
    bool canRender() const { return m_useRhi; }

    QRhi *rhi() const { return m_rhi.get(); }

    void init();
    void render();

    void initRhi();

    void resizeSwapChain();
    void releaseSwapChain();

    void updateTextures(QRhiResourceUpdateBatch *rub);
    void updateSubtitle(QRhiResourceUpdateBatch *rub, const QSize &frameSize);
    void freeTextures();

    void setupGraphicsPipeline(QRhiGraphicsPipeline *pipeline, QRhiShaderResourceBindings *bindings, QVideoFrameFormat::PixelFormat fmt);

    QVideoWindow *q = nullptr;
    Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio;

    QBackingStore *backingStore = nullptr;

#if QT_CONFIG(opengl)
    std::unique_ptr<QOffscreenSurface> m_fallbackSurface;
#endif
    std::unique_ptr<QRhi> m_rhi;
    std::unique_ptr<QRhiSwapChain> m_swapChain;
    std::unique_ptr<QRhiRenderPassDescriptor> m_renderPass;

    std::unique_ptr<QRhiBuffer> m_vertexBuf;
    bool m_vertexBufReady = false;
    std::unique_ptr<QRhiBuffer> m_uniformBuf;
    QRhiTexture *m_frameTextures[3] = {};
    std::unique_ptr<QRhiSampler> m_textureSampler;
    std::unique_ptr<QRhiShaderResourceBindings> m_shaderResourceBindings;
    std::unique_ptr<QRhiGraphicsPipeline> m_graphicsPipeline;

    std::unique_ptr<QRhiTexture> m_subtitleTexture;
    std::unique_ptr<QRhiShaderResourceBindings> m_subtitleResourceBindings;
    std::unique_ptr<QRhiGraphicsPipeline> m_subtitlePipeline;
    std::unique_ptr<QRhiBuffer> m_subtitleUniformBuf;

    std::unique_ptr<QVideoSink> m_sink;
    QRhi::Implementation m_graphicsApi = QRhi::Null;
    QSize m_frameSize = QSize(-1, -1);
    QVideoFrame m_currentFrame;
    QVideoTextureHelper::SubtitleLayout m_subtitleLayout;

    enum { NVideoFrameSlots = 4 };
    QVideoFrame m_videoFrameSlots[NVideoFrameSlots];

    bool initialized = false;
    bool isExposed = false;
    bool m_useRhi = true;
    bool m_hasSwapChain = false;
    bool m_texturesDirty = true;
    bool m_subtitleDirty = false;
    bool m_hasSubtitle = false;
    QVideoFrameFormat::PixelFormat format = QVideoFrameFormat::Format_Invalid;
};

class Q_MULTIMEDIA_EXPORT QVideoWindow : public QWindow
{
    Q_OBJECT
public:
    explicit QVideoWindow(QScreen *screen = nullptr);
    explicit QVideoWindow(QWindow *parent);
    ~QVideoWindow();

    Q_INVOKABLE QVideoSink *videoSink() const;

    Qt::AspectRatioMode aspectRatioMode() const;

public Q_SLOTS:
    void setAspectRatioMode(Qt::AspectRatioMode mode);

Q_SIGNALS:
    void aspectRatioModeChanged(Qt::AspectRatioMode mode);

protected:
    bool event(QEvent *e) override;
    void resizeEvent(QResizeEvent *) override;

private Q_SLOTS:
    void setVideoFrame(const QVideoFrame &frame);

private:
    friend class QVideoWindowPrivate;
    std::unique_ptr<QVideoWindowPrivate> d;
};

QT_END_NAMESPACE

#endif
