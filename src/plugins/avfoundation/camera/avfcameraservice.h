/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef AVFCAMERASERVICE_H
#define AVFCAMERASERVICE_H

#include <QtCore/qobject.h>
#include <QtCore/qset.h>
#include <qmediaservice.h>


QT_BEGIN_NAMESPACE
class QCameraControl;
class AVFCameraControl;
class AVFCameraMetaDataControl;
class AVFVideoWindowControl;
class AVFVideoWidgetControl;
class AVFVideoRendererControl;
class AVFMediaRecorderControl;
class AVFImageCaptureControl;
class AVFCameraSession;
class AVFVideoDeviceControl;
class AVFAudioInputSelectorControl;

class AVFCameraService : public QMediaService
{
Q_OBJECT
public:
    AVFCameraService(QObject *parent = 0);
    ~AVFCameraService();

    QMediaControl* requestControl(const char *name);
    void releaseControl(QMediaControl *control);

    AVFCameraSession *session() const { return m_session; }
    AVFCameraControl *cameraControl() const { return m_cameraControl; }
    AVFVideoDeviceControl *videoDeviceControl() const { return m_videoDeviceControl; }
    AVFAudioInputSelectorControl *audioInputSelectorControl() const { return m_audioInputSelectorControl; }
    AVFCameraMetaDataControl *metaDataControl() const { return m_metaDataControl; }
    AVFMediaRecorderControl *recorderControl() const { return m_recorderControl; }
    AVFImageCaptureControl *imageCaptureControl() const { return m_imageCaptureControl; }


private:
    AVFCameraSession *m_session;
    AVFCameraControl *m_cameraControl;
    AVFVideoDeviceControl *m_videoDeviceControl;
    AVFAudioInputSelectorControl *m_audioInputSelectorControl;
    AVFVideoRendererControl *m_videoOutput;
    AVFCameraMetaDataControl *m_metaDataControl;
    AVFMediaRecorderControl *m_recorderControl;
    AVFImageCaptureControl *m_imageCaptureControl;
};

QT_END_NAMESPACE

#endif
