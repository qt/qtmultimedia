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

#ifndef AVFCAMERASERVICE_H
#define AVFCAMERASERVICE_H

#include <QtCore/qobject.h>
#include <QtCore/qset.h>
#include <qmediaservice.h>


QT_BEGIN_NAMESPACE
class QCameraControl;
class QMediaRecorderControl;
class AVFCameraControl;
class AVFCameraInfoControl;
class AVFCameraMetaDataControl;
class AVFVideoWindowControl;
class AVFVideoWidgetControl;
class AVFCameraRendererControl;
class AVFImageCaptureControl;
class AVFCameraSession;
class AVFCameraDeviceControl;
class AVFAudioInputSelectorControl;
class AVFCameraFocusControl;
class AVFCameraExposureControl;
class AVFCameraZoomControl;
class AVFCameraViewfinderSettingsControl2;
class AVFCameraViewfinderSettingsControl;
class AVFImageEncoderControl;
class AVFCameraFlashControl;
class AVFMediaRecorderControl;
class AVFMediaRecorderControlIOS;

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
    AVFCameraDeviceControl *videoDeviceControl() const { return m_videoDeviceControl; }
    AVFAudioInputSelectorControl *audioInputSelectorControl() const { return m_audioInputSelectorControl; }
    AVFCameraMetaDataControl *metaDataControl() const { return m_metaDataControl; }
    AVFMediaRecorderControl *recorderControl() const;
    AVFMediaRecorderControlIOS *recorderControlIOS() const;
    AVFImageCaptureControl *imageCaptureControl() const { return m_imageCaptureControl; }
    AVFCameraFocusControl *cameraFocusControl() const { return m_cameraFocusControl; }
    AVFCameraExposureControl *cameraExposureControl() const {return m_cameraExposureControl; }
    AVFCameraZoomControl *cameraZoomControl() const {return m_cameraZoomControl; }
    AVFCameraRendererControl *videoOutput() const {return m_videoOutput; }
    AVFCameraViewfinderSettingsControl2 *viewfinderSettingsControl2() const {return m_viewfinderSettingsControl2; }
    AVFCameraViewfinderSettingsControl *viewfinderSettingsControl() const {return m_viewfinderSettingsControl; }
    AVFImageEncoderControl *imageEncoderControl() const {return m_imageEncoderControl; }
    AVFCameraFlashControl *flashControl() const {return m_flashControl; }

private:
    AVFCameraSession *m_session;
    AVFCameraControl *m_cameraControl;
    AVFCameraInfoControl *m_cameraInfoControl;
    AVFCameraDeviceControl *m_videoDeviceControl;
    AVFAudioInputSelectorControl *m_audioInputSelectorControl;
    AVFCameraRendererControl *m_videoOutput;
    AVFCameraMetaDataControl *m_metaDataControl;
    QMediaRecorderControl *m_recorderControl;
    AVFImageCaptureControl *m_imageCaptureControl;
    AVFCameraFocusControl *m_cameraFocusControl;
    AVFCameraExposureControl *m_cameraExposureControl;
    AVFCameraZoomControl *m_cameraZoomControl;
    AVFCameraViewfinderSettingsControl2 *m_viewfinderSettingsControl2;
    AVFCameraViewfinderSettingsControl *m_viewfinderSettingsControl;
    AVFImageEncoderControl *m_imageEncoderControl;
    AVFCameraFlashControl *m_flashControl;
};

QT_END_NAMESPACE

#endif
