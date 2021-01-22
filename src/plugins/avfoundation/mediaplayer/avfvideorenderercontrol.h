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

#ifndef AVFVIDEORENDERERCONTROL_H
#define AVFVIDEORENDERERCONTROL_H

#include <QtMultimedia/QVideoRendererControl>
#include <QtCore/QMutex>
#include <QtCore/QSize>

#include "avfvideooutput.h"

#import <CoreVideo/CVBase.h>

QT_BEGIN_NAMESPACE

class AVFDisplayLink;
class AVFVideoFrameRenderer;

class AVFVideoRendererControl : public QVideoRendererControl, public AVFVideoOutput
{
    Q_OBJECT
    Q_INTERFACES(AVFVideoOutput)
public:
    explicit AVFVideoRendererControl(QObject *parent = nullptr);
    virtual ~AVFVideoRendererControl();

    QAbstractVideoSurface *surface() const override;
    void setSurface(QAbstractVideoSurface *surface) override;

    void setLayer(void *playerLayer) override;

private Q_SLOTS:
    void updateVideoFrame(const CVTimeStamp &ts);

Q_SIGNALS:
    void surfaceChanged(QAbstractVideoSurface *surface);

private:
    void setupVideoOutput();

    QMutex m_mutex;
    QAbstractVideoSurface *m_surface;

    void *m_playerLayer;

    AVFVideoFrameRenderer *m_frameRenderer;
    AVFDisplayLink *m_displayLink;
    QSize m_nativeSize;
    bool m_enableOpenGL;
};

QT_END_NAMESPACE

#endif // AVFVIDEORENDERERCONTROL_H
