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

#ifndef QGSTREAMERVIDEOWINDOW_H
#define QGSTREAMERVIDEOWINDOW_H

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

#include <private/qgstpipeline_p.h>
#include <private/qgstreamervideooverlay_p.h>
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
