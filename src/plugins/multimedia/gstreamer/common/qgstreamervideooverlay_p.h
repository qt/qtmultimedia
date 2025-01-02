// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERVIDEOOVERLAY_P_H
#define QGSTREAMERVIDEOOVERLAY_P_H

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

#include <qgstpipeline_p.h>
#include <qgstreamerbufferprobe_p.h>
#include <qgst_p.h>
#include <QtGui/qwindowdefs.h>

QT_BEGIN_NAMESPACE
class QGstreamerVideoSink;

class Q_MULTIMEDIA_EXPORT QGstreamerVideoOverlay
        : public QObject
        , public QGstreamerSyncMessageFilter
        , private QGstreamerBufferProbe
{
    Q_OBJECT
public:
    explicit QGstreamerVideoOverlay(QGstreamerVideoSink *parent = 0, const QByteArray &elementName = QByteArray());
    virtual ~QGstreamerVideoOverlay();

    QGstElement videoSink() const;
    void setVideoSink(QGstElement);
    QSize nativeVideoSize() const;

    void setWindowHandle(WId id);
    void setRenderRectangle(const QRect &rect);

    void setAspectRatioMode(Qt::AspectRatioMode mode);
    void setFullScreen(bool fullscreen);

    bool processSyncMessage(const QGstreamerMessage &message) override;

    bool isNull() const { return m_videoSink.isNull(); }

Q_SIGNALS:
    void nativeVideoSizeChanged();
    void activeChanged();

private:
    void probeCaps(GstCaps *caps) override;
    void applyRenderRect();

    QGstreamerVideoSink *m_gstreamerVideoSink = nullptr;
    QGstElement m_videoSink;
    QSize m_nativeVideoSize;

    bool m_hasForceAspectRatio = false;
    bool m_hasFullscreen = false;
    Qt::AspectRatioMode m_aspectRatioMode = Qt::KeepAspectRatio;
    bool m_fullScreen = false;

    WId m_windowId = 0;
    QRect renderRect;
};

QT_END_NAMESPACE

#endif // QGSTREAMERVIDEOOVERLAY_P_H

