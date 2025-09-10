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

#include <QtMultimedia/qvideosink.h>
#include <QtMultimedia/private/qplatformvideosink_p.h>

#include <common/qgstvideorenderersink_p.h>
#include <common/qgstpipeline_p.h>

QT_BEGIN_NAMESPACE

class QGstreamerVideoSink : public QPlatformVideoSink
{
    Q_OBJECT

public:
    explicit QGstreamerVideoSink(QVideoSink *parent = nullptr);
    ~QGstreamerVideoSink() override;

    void setRhi(QRhi *rhi) override;
    QRhi *rhi() const { return m_rhi; }

    QGstElement gstSink();

    GstContext *gstGlDisplayContext() const { return m_gstGlDisplayContext.get(); }
    GstContext *gstGlLocalContext() const { return m_gstGlLocalContext.get(); }
    Qt::HANDLE eglDisplay() const { return m_eglDisplay; }
    QFunctionPointer eglImageTargetTexture2D() const { return m_eglImageTargetTexture2D; }

    void setActive(bool);
    void setAsync(bool);

Q_SIGNALS:
    void aboutToBeDestroyed();

private:
    void createQtSink();
    void updateSinkElement(QGstVideoRendererSinkElement newSink);

    void unrefGstContexts();
    void updateGstContexts();

    QGstBin m_sinkBin;
    QGstElement m_gstPreprocess;
    QGstElement m_gstCapsFilter;
    QGstElement m_gstVideoSink;
    QGstVideoRendererSinkElement m_gstQtSink;

    QRhi *m_rhi = nullptr;
    bool m_isActive = true;
    bool m_sinkIsAsync = true;

    Qt::HANDLE m_eglDisplay = nullptr;
    QFunctionPointer m_eglImageTargetTexture2D = nullptr;

    QGstContextHandle m_gstGlLocalContext;
    QGstContextHandle m_gstGlDisplayContext;
};

QT_END_NAMESPACE

#endif
