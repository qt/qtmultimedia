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

#ifndef S60CAMERACONTROL_H
#define S60CAMERACONTROL_H

#include <qcameracontrol.h>

#include "s60cameraengineobserver.h"    // MCameraEngineObserver
#include "s60videocapturesession.h"     // TVideoCaptureState
#include "s60cameraviewfinderengine.h"  // ViewfinderOutputType

#include <e32base.h>
#include <fbs.h>

QT_USE_NAMESPACE

class S60CameraService;
class S60ImageCaptureSession;
class S60VideoCaptureSession;
class S60CameraSettings;
class CCameraEngine;
class S60CameraViewfinderEngine;
class QTimer;

/*
 * Control for controlling camera base operations (e.g. start/stop and capture
 * mode).
 */
class S60CameraControl : public QCameraControl, public MCameraEngineObserver
{
    Q_OBJECT

public: // Constructors & Destructor

    S60CameraControl(QObject *parent = 0);
    S60CameraControl(S60VideoCaptureSession *videosession,
                     S60ImageCaptureSession *imagesession,
                     QObject *parent = 0);
    ~S60CameraControl();

public: // QCameraControl

    // State
    QCamera::State state() const;
    void setState(QCamera::State state);

    // Status
    QCamera::Status status() const;

    // Capture Mode
    QCamera::CaptureMode captureMode() const;
    void setCaptureMode(QCamera::CaptureMode);
    bool isCaptureModeSupported(QCamera::CaptureMode mode) const;

    // Property Setting
    bool canChangeProperty(QCameraControl::PropertyChangeType changeType, QCamera::Status status) const;

/*
Q_SIGNALS:
    void stateChanged(QCamera::State);
    void statusChanged(QCamera::Status);
    void error(int error, const QString &errorString);
    void captureModeChanged(QCamera::CaptureMode);
*/

public: // Internal

    void setError(const TInt error, const QString &description);
    void resetCameraOrientation();

    // To provide QVideoDeviceControl info
    static int deviceCount();
    static QString name(const int index);
    static QString description(const int index);
    int defaultDevice() const;
    int selectedDevice() const;
    void setSelectedDevice(const int index);

    void setVideoOutput(QObject *output,
                        const S60CameraViewfinderEngine::ViewfinderOutputType type);
    void releaseVideoOutput(const S60CameraViewfinderEngine::ViewfinderOutputType type);

private slots: // Internal Slots

    void videoStateChanged(const S60VideoCaptureSession::TVideoCaptureState state);
    // Needed to detect image capture completion when trying to rotate the camera
    void imageCaptured(const int imageId, const QImage& preview);
    /*
     * This method moves the camera to the StandBy status:
     *    - If camera access was lost
     *    - If camera has been inactive in LoadedStatus for a long time
     */
    void toStandByStatus();
    void advancedSettingsCreated();

protected: // MCameraEngineObserver

    void MceoCameraReady();
    void MceoHandleError(TCameraEngineError aErrorType, TInt aError);

private: // Internal

    QCamera::Error fromSymbianErrorToQtMultimediaError(int aError);

    void loadCamera();
    void unloadCamera();
    void startCamera();
    void stopCamera();

    void resetCamera(bool errorHandling = false);
    void setCameraHandles();

signals: // Internal Signals

    void cameraReadyChanged(bool);
    void devicesChanged();

private: // Data

    CCameraEngine               *m_cameraEngine;
    S60CameraViewfinderEngine   *m_viewfinderEngine;
    S60ImageCaptureSession      *m_imageSession;
    S60VideoCaptureSession      *m_videoSession;
    S60CameraSettings           *m_advancedSettings;
    QObject                     *m_videoOutput;
    QTimer                      *m_inactivityTimer;
    QCamera::CaptureMode        m_captureMode;
    QCamera::CaptureMode        m_requestedCaptureMode;
    bool                        m_settingCaptureModeInternally;
    QCamera::Status             m_internalState;
    QCamera::State              m_requestedState;
    int                         m_deviceIndex;
    mutable int                 m_error;
    bool                        m_changeCaptureModeWhenReady;
    bool                        m_rotateCameraWhenReady;
    S60VideoCaptureSession::TVideoCaptureState m_videoCaptureState;
};

#endif // S60CAMERACONTROL_H
