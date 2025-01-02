// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERVIDEOSINK_H
#define QGSTREAMERVIDEOSINK_H

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

#include <private/qtmultimediaglobal_p.h>
#include <private/qplatformvideosink_p.h>

#include <qgstpipeline_p.h>
#include <qgstreamervideooverlay_p.h>
#include <QtGui/qcolor.h>
#include <qvideosink.h>

#if QT_CONFIG(gstreamer_gl)
#include <gst/gl/gl.h>
#endif

QT_BEGIN_NAMESPACE
class QGstreamerVideoRenderer;
class QVideoWindow;

class Q_MULTIMEDIA_EXPORT QGstreamerVideoSink
    : public QPlatformVideoSink
{
    Q_OBJECT
public:
    explicit QGstreamerVideoSink(QVideoSink *parent = 0);
    ~QGstreamerVideoSink();

    void setRhi(QRhi *rhi) override;
    QRhi *rhi() const { return m_rhi; }

    QGstElement gstSink();
    QGstElement subtitleSink() const { return gstSubtitleSink; }

    void setPipeline(QGstPipeline pipeline);
    bool inStoppedState() const;

    GstContext *gstGlDisplayContext() const { return m_gstGlDisplayContext; }
    GstContext *gstGlLocalContext() const { return m_gstGlLocalContext; }
    Qt::HANDLE eglDisplay() const { return m_eglDisplay; }
    QFunctionPointer eglImageTargetTexture2D() const { return m_eglImageTargetTexture2D; }

private:
    void createQtSink();
    void updateSinkElement();

    void unrefGstContexts();
    void updateGstContexts();

    QGstPipeline gstPipeline;
    QGstBin sinkBin;
    QGstElement gstQueue;
    QGstElement gstPreprocess;
    QGstElement gstVideoSink;
    QGstElement gstQtSink;
    QGstElement gstSubtitleSink;

    QRhi *m_rhi = nullptr;

    Qt::HANDLE m_eglDisplay = nullptr;
    QFunctionPointer m_eglImageTargetTexture2D = nullptr;
    GstContext *m_gstGlLocalContext = nullptr;
    GstContext *m_gstGlDisplayContext = nullptr;
};

QT_END_NAMESPACE

#endif
