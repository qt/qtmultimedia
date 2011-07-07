/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef S60CAMERASERVICE_H
#define S60CAMERASERVICE_H

#include <QtCore/qobject.h>
#include <qmediaservice.h>

QT_USE_NAMESPACE

class S60MediaContainerControl;
class S60VideoEncoderControl;
class S60AudioEncoderControl;
class S60CameraControl;
class S60VideoDeviceControl;
class S60MediaRecorderControl;
class S60ImageCaptureSession;
class S60VideoCaptureSession;
class S60CameraFocusControl;
class S60CameraExposureControl;
class S60CameraFlashControl;
class S60CameraImageProcessingControl;
class S60CameraImageCaptureControl;
class S60VideoWidgetControl;
class S60ImageEncoderControl;
class S60CameraLocksControl;
class S60VideoRendererControl;
class S60VideoWindowControl;

class S60CameraService : public QMediaService
{
    Q_OBJECT

public: // Contructor & Destructor

    S60CameraService(QObject *parent = 0);
    ~S60CameraService();

public: // QMediaService

    QMediaControl *requestControl(const char *name);
    void releaseControl(QMediaControl *control);

public: // Static Device Info

    static int deviceCount();
    static QString deviceName(const int index);
    static QString deviceDescription(const int index);

private: // Data

    S60ImageCaptureSession          *m_imagesession;
    S60VideoCaptureSession          *m_videosession;
    S60MediaContainerControl        *m_mediaFormat;
    S60VideoEncoderControl          *m_videoEncoder;
    S60AudioEncoderControl          *m_audioEncoder;
    S60CameraControl                *m_control;
    S60VideoDeviceControl           *m_videoDeviceControl;
    S60CameraFocusControl           *m_focusControl;
    S60CameraExposureControl        *m_exposureControl;
    S60CameraFlashControl           *m_flashControl;
    S60CameraImageProcessingControl *m_imageProcessingControl;
    S60CameraImageCaptureControl    *m_imageCaptureControl;
    S60MediaRecorderControl         *m_media;
    S60VideoWidgetControl           *m_viewFinderWidget;
    S60ImageEncoderControl          *m_imageEncoderControl;
    S60CameraLocksControl           *m_locksControl;
    S60VideoRendererControl         *m_rendererControl;
    S60VideoWindowControl           *m_windowControl;
};

#endif // S60CAMERASERVICE_H
