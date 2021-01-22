/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
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

#ifndef QWINRTCAMERAVIDEORENDERERCONTROL_H
#define QWINRTCAMERAVIDEORENDERERCONTROL_H

#include "qwinrtabstractvideorenderercontrol.h"

#include <QVideoFrame>

struct IMF2DBuffer;

QT_BEGIN_NAMESPACE

class QWinRTVideoProbeControl;
class QVideoSurfaceFormat;
class QWinRTCameraVideoRendererControlPrivate;
class QWinRTCameraVideoRendererControl : public QWinRTAbstractVideoRendererControl
{
    Q_OBJECT
public:
    explicit QWinRTCameraVideoRendererControl(const QSize &size, QObject *parent);
    ~QWinRTCameraVideoRendererControl() override;

    bool render(ID3D11Texture2D *texture) override;
    bool dequeueFrame(QVideoFrame *frame) override;
    void queueBuffer(IMF2DBuffer *buffer);
    void discardBuffers();
    void incrementProbe();
    void decrementProbe();

signals:
    void bufferRequested();
    void videoFrameProbed(const QVideoFrame &frame);

public slots:
    void resetSampleFormat();

private:
    QScopedPointer<QWinRTCameraVideoRendererControlPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTCameraVideoRendererControl)
};

QT_END_NAMESPACE

#endif // QWINRTCAMERAVIDEORENDERERCONTROL_H
