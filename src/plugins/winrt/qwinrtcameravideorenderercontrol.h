/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
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
    ~QWinRTCameraVideoRendererControl();

    bool render(ID3D11Texture2D *texture) Q_DECL_OVERRIDE;
    bool dequeueFrame(QVideoFrame *frame) Q_DECL_OVERRIDE;
    void queueBuffer(IMF2DBuffer *buffer);
    void discardBuffers();
    void incrementProbe();
    void decrementProbe();

signals:
    void bufferRequested();
    void videoFrameProbed(const QVideoFrame &frame);

private:
    QScopedPointer<QWinRTCameraVideoRendererControlPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTCameraVideoRendererControl)
};

QT_END_NAMESPACE

#endif // QWINRTCAMERAVIDEORENDERERCONTROL_H
