/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
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
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef AVFCAMERARENDERERCONTROL_H
#define AVFCAMERARENDERERCONTROL_H

#include <QtMultimedia/qvideorenderercontrol.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtCore/qmutex.h>

#import <AVFoundation/AVFoundation.h>

@class AVFCaptureFramesDelegate;

QT_BEGIN_NAMESPACE

class AVFCameraSession;
class AVFCameraService;
class AVFCameraRendererControl;

class AVFCameraRendererControl : public QVideoRendererControl
{
Q_OBJECT
public:
    AVFCameraRendererControl(QObject *parent = 0);
    ~AVFCameraRendererControl();

    QAbstractVideoSurface *surface() const;
    void setSurface(QAbstractVideoSurface *surface);

    void configureAVCaptureSession(AVFCameraSession *cameraSession);
    void syncHandleViewfinderFrame(const QVideoFrame &frame);

    AVCaptureVideoDataOutput *videoDataOutput() const;

#ifdef Q_OS_IOS
    AVFCaptureFramesDelegate *captureDelegate() const;
    void resetCaptureDelegate() const;
#endif

Q_SIGNALS:
    void surfaceChanged(QAbstractVideoSurface *surface);

private Q_SLOTS:
    void handleViewfinderFrame();
    void updateCaptureConnection();

private:
    QAbstractVideoSurface *m_surface;
    AVFCaptureFramesDelegate *m_viewfinderFramesDelegate;
    AVFCameraSession *m_cameraSession;
    AVCaptureVideoDataOutput *m_videoDataOutput;

    bool m_needsHorizontalMirroring;

    QVideoFrame m_lastViewfinderFrame;
    QMutex m_vfMutex;
    dispatch_queue_t m_delegateQueue;
};

QT_END_NAMESPACE

#endif
