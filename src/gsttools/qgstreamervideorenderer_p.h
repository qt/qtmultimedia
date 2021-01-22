/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QGSTREAMERVIDEORENDERER_H
#define QGSTREAMERVIDEORENDERER_H

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

#include <private/qgsttools_global_p.h>
#include <qvideorenderercontrol.h>
#include <private/qvideosurfacegstsink_p.h>
#include <qabstractvideosurface.h>

#include "qgstreamervideorendererinterface_p.h"

QT_BEGIN_NAMESPACE

class Q_GSTTOOLS_EXPORT QGstreamerVideoRenderer : public QVideoRendererControl, public QGstreamerVideoRendererInterface
{
    Q_OBJECT
    Q_INTERFACES(QGstreamerVideoRendererInterface)
public:
    QGstreamerVideoRenderer(QObject *parent = 0);
    virtual ~QGstreamerVideoRenderer();

    QAbstractVideoSurface *surface() const override;
    void setSurface(QAbstractVideoSurface *surface) override;

    GstElement *videoSink() override;
    void setVideoSink(GstElement *) override;

    void stopRenderer() override;
    bool isReady() const override { return m_surface != 0; }

signals:
    void sinkChanged();
    void readyChanged(bool);

private slots:
    void handleFormatChange();

private:
    GstElement *m_videoSink = nullptr;
    QPointer<QAbstractVideoSurface> m_surface;
};

QT_END_NAMESPACE

#endif // QGSTREAMERVIDEORENDRER_H
