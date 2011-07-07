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

#include <QtCore/qstring.h>
#include <QTimer>

#include "s60cameraservice.h"
#include "s60cameraengine.h"
#include "s60cameracontrol.h"
#include "s60imagecapturesession.h"
#include "s60videowidgetcontrol.h"
#include "s60cameraviewfinderengine.h"
#include "s60cameraconstants.h"

S60CameraControl::S60CameraControl(QObject *parent) :
    QCameraControl(parent)
{
}

S60CameraControl::S60CameraControl(S60VideoCaptureSession *videosession,
                                   S60ImageCaptureSession *imagesession,
                                   QObject *parent):
    QCameraControl(parent),
    m_cameraEngine(0),
    m_viewfinderEngine(0),
    m_imageSession(0),
    m_videoSession(0),
    m_advancedSettings(0),
    m_videoOutput(0),
    m_inactivityTimer(0),
    m_captureMode(QCamera::CaptureStillImage),  // Default CaptureMode
    m_requestedCaptureMode(QCamera::CaptureStillImage),
    m_settingCaptureModeInternally(false),
    m_internalState(QCamera::UnloadedStatus),   // Default Status
    m_requestedState(QCamera::UnloadedState),   // Default State
    m_deviceIndex(KDefaultCameraDevice),
    m_error(KErrNone),
    m_changeCaptureModeWhenReady(false),
    m_rotateCameraWhenReady(false),
    m_videoCaptureState(S60VideoCaptureSession::ENotInitialized)
{
    m_videoSession = videosession;
    m_imageSession = imagesession;

    m_inactivityTimer = new QTimer;
    if (m_inactivityTimer)
        m_inactivityTimer->setSingleShot(true);

    TRAPD(err, m_cameraEngine = CCameraEngine::NewL(m_deviceIndex, KECamCameraPriority, this));
    if (err) {
        m_error = err;
        if (err == KErrPermissionDenied)
            qWarning("Failed to create camera. Possibly missing capabilities.");
        else
            qWarning("Failed to create camera.");
        return;
    }

    m_viewfinderEngine = new S60CameraViewfinderEngine(this, m_cameraEngine, this);
    if (m_viewfinderEngine == 0) {
        m_error = KErrNoMemory;
        qWarning("Failed to create viewfinder engine.");
        return;
    }

    // Connect signals
    connect(m_inactivityTimer, SIGNAL(timeout()), this, SLOT(toStandByStatus()));
    connect(this, SIGNAL(statusChanged(QCamera::Status)),
        m_imageSession, SLOT(cameraStatusChanged(QCamera::Status)));
    connect(this, SIGNAL(statusChanged(QCamera::Status)),
        m_videoSession, SLOT(cameraStatusChanged(QCamera::Status)));
    connect(m_videoSession, SIGNAL(stateChanged(S60VideoCaptureSession::TVideoCaptureState)),
        this, SLOT(videoStateChanged(S60VideoCaptureSession::TVideoCaptureState)));
    connect(m_imageSession, SIGNAL(advancedSettingChanged()), this, SLOT(advancedSettingsCreated()));
    connect(this, SIGNAL(cameraReadyChanged(bool)), m_imageSession, SIGNAL(readyForCaptureChanged(bool)));
    connect(m_viewfinderEngine, SIGNAL(error(int, const QString&)), this, SIGNAL(error(int,const QString&)));
    connect(m_imageSession, SIGNAL(cameraError(int, const QString&)), this, SIGNAL(error(int, const QString&)));
    connect(m_imageSession, SIGNAL(captureSizeChanged(const QSize&)),
        m_viewfinderEngine, SLOT(handleContentAspectRatioChange(const QSize&)));
    connect(m_videoSession, SIGNAL(captureSizeChanged(const QSize&)),
        m_viewfinderEngine, SLOT(handleContentAspectRatioChange(const QSize&)));

    setCameraHandles();
}

S60CameraControl::~S60CameraControl()
{
    unloadCamera();

    if (m_viewfinderEngine) {
        delete m_viewfinderEngine;
        m_viewfinderEngine = 0;
    }

    // Make sure AdvancedSettings are destructed
    m_imageSession->deleteAdvancedSettings();

    if (m_cameraEngine) {
        delete m_cameraEngine;
        m_cameraEngine = 0;
    }

    if (m_inactivityTimer) {
        delete m_inactivityTimer;
        m_inactivityTimer = 0;
    }
}

void S60CameraControl::setState(QCamera::State state)
{
    if (m_error) { // Most probably failure in contructor
        setError(m_error, tr("Unexpected camera error."));
        return;
    }

    if (m_requestedState == state)
        return;

    if (m_inactivityTimer->isActive())
        m_inactivityTimer->stop();

    // Save the target state
    m_requestedState = state;
    emit stateChanged(m_requestedState);

    switch (state) {
        case QCamera::UnloadedState: // To UnloadedState - Release resources
            switch (m_internalState) {
                case QCamera::UnloadedStatus:
                    // Do nothing
                    break;
                case QCamera::LoadingStatus:
                case QCamera::StartingStatus:
                    // Release resources when ready  (setting state handles this)
                    return;
                case QCamera::LoadedStatus:
                case QCamera::StandbyStatus:
                    // Unload
                    unloadCamera();
                    break;
                case QCamera::ActiveStatus:
                    // Stop and Unload
                    stopCamera();
                    unloadCamera();
                    break;

                default:
                    // Unrecognized internal state (Status)
                    setError(KErrGeneral, tr("Unexpected camera error."));
                    return;
            }
            break;

        case QCamera::LoadedState: // To LoadedState - Reserve resources OR Stop ViewFinder and Cancel Capture
            switch (m_internalState) {
                case QCamera::UnloadedStatus:
                case QCamera::StandbyStatus:
                    // Load
                    loadCamera();
                    break;
                case QCamera::LoadingStatus:
                    // Discard, already moving to LoadedStatus
                    return;
                case QCamera::StartingStatus:
                    // Stop when ready  (setting state handles this)
                    return;
                case QCamera::LoadedStatus:
                    m_inactivityTimer->start(KInactivityTimerTimeout);
                    break;
                case QCamera::ActiveStatus:
                    // Stop
                    stopCamera();
                    break;

                default:
                    // Unregocnized internal state (Status)
                    setError(KErrGeneral, tr("Unexpected camera error."));
                    return;
            }
            break;

        case QCamera::ActiveState: // To ActiveState - (Reserve Resources and) Start ViewFinder
            switch (m_internalState) {
                case QCamera::UnloadedStatus:
                case QCamera::StandbyStatus:
                    // Load and Start (setting state handles starting)
                    loadCamera();
                    break;
                case QCamera::LoadingStatus:
                    // Start when loaded (setting state handles this)
                    break;
                case QCamera::StartingStatus:
                    // Discard, already moving to ActiveStatus
                    return;
                case QCamera::LoadedStatus:
                    // Start
                    startCamera();
                    break;
                case QCamera::ActiveStatus:
                    // Do nothing
                    break;

                default:
                    // Unregocnized internal state (Status)
                    setError(KErrGeneral, tr("Unexpected camera error."));
                    return;
            }
            break;

        default:
            setError(KErrNotSupported, tr("Requested state is not supported."));
            return;
    }
}

QCamera::State S60CameraControl::state() const
{
    return m_requestedState;
}

QCamera::Status S60CameraControl::status() const
{
    return m_internalState;
}

QCamera::CaptureMode S60CameraControl::captureMode() const
{
    return m_captureMode;
}

void S60CameraControl::setCaptureMode(QCamera::CaptureMode mode)
{
    if (m_error) { // Most probably failure in contructor
        setError(m_error, tr("Unexpected camera error."));
        return;
    }

    if (m_captureMode == mode)
        return;

    // Setting CaptureMode Internally or Externally (Client)
    if (!m_settingCaptureModeInternally) {
        // Save the requested mode
        m_requestedCaptureMode = mode;

        // CaptureMode change pending (backend busy), wait
        if (m_changeCaptureModeWhenReady)
            return;
    } else {
        m_changeCaptureModeWhenReady = false; // Reset
    }
    m_settingCaptureModeInternally = false; // Reset

    if (!isCaptureModeSupported(mode)) {
        setError(KErrNotSupported, tr("Requested capture mode is not supported."));
        return;
    }

    if (m_inactivityTimer->isActive())
        m_inactivityTimer->stop();

    switch (m_internalState) {
        case QCamera::UnloadedStatus:
        case QCamera::LoadedStatus:
        case QCamera::StandbyStatus:
            switch (mode) {
                case QCamera::CaptureStillImage:
                    m_videoSession->releaseVideoRecording();
                    m_captureMode = QCamera::CaptureStillImage;
                    if (m_internalState == QCamera::LoadedStatus)
                        m_inactivityTimer->start(KInactivityTimerTimeout);
                    else if (m_internalState == QCamera::StandbyStatus)
                        loadCamera();
                    break;
                case QCamera::CaptureVideo:
                    m_imageSession->releaseImageCapture();
                    m_captureMode = QCamera::CaptureVideo;
                    if (m_internalState == QCamera::LoadedStatus) {
                        // Revet InternalState as we need to wait for the video
                        // side initialization to complete
                        m_internalState = QCamera::LoadingStatus;
                        emit statusChanged(m_internalState);
                        int prepareSuccess = m_videoSession->initializeVideoRecording();
                        setError(prepareSuccess, tr("Loading video capture failed."));
                    } else if (m_internalState == QCamera::StandbyStatus)
                        loadCamera();
                    break;
            }
            break;
        case QCamera::LoadingStatus:
        case QCamera::StartingStatus:
            m_changeCaptureModeWhenReady = true;
            return;
        case QCamera::ActiveStatus:
            // Stop, Change Mode and Start again
            stopCamera();
            switch (mode) {
                case QCamera::CaptureStillImage:
                    m_videoSession->releaseVideoRecording();
                    m_captureMode = QCamera::CaptureStillImage;
                    startCamera();
                    break;
                case QCamera::CaptureVideo:
                    m_imageSession->releaseImageCapture();
                    m_captureMode = QCamera::CaptureVideo;
                    // Revet InternalState as we need to wait for the video
                    // side initialization to complete
                    m_internalState = QCamera::LoadingStatus;
                    emit statusChanged(m_internalState);
                    int prepareSuccess = m_videoSession->initializeVideoRecording();
                    setError(prepareSuccess, tr("Loading video recorder failed."));
                    break;
            }
            break;

        default:
            // Unregocnized internal state (Status)
            setError(KErrNotSupported, tr("Requested capture mode is not supported."));
            break;
    }

    emit captureModeChanged(mode);
}

bool S60CameraControl::isCaptureModeSupported(QCamera::CaptureMode mode) const
{
    switch (mode) {
        case QCamera::CaptureStillImage:
            return true;
        case QCamera::CaptureVideo:
            return true;

        default:
            return false;
    }
}

bool S60CameraControl::canChangeProperty(QCameraControl::PropertyChangeType changeType, QCamera::Status status) const
{
    Q_UNUSED(status);

    bool returnValue = false;

    switch (changeType) {
        case QCameraControl::CaptureMode:
        case QCameraControl::VideoEncodingSettings:
        case QCameraControl::ImageEncodingSettings:
            returnValue = true;
            break;

        case QCameraControl::Viewfinder:
            returnValue = false;
            break;

        default:
            // Safer to revert state before the unknown operation
            returnValue = false;
            break;
    }

    return returnValue;
}

void S60CameraControl::setVideoOutput(QObject *output,
                                      S60CameraViewfinderEngine::ViewfinderOutputType type)
{
    if (!m_viewfinderEngine) {
        setError(KErrGeneral, tr("Failed to set viewfinder"));
        return;
    }

    switch (type) {
    case S60CameraViewfinderEngine::OutputTypeVideoWidget:
        m_viewfinderEngine->setVideoWidgetControl(output);
        break;
    case S60CameraViewfinderEngine::OutputTypeRenderer:
        m_viewfinderEngine->setVideoRendererControl(output);
        break;
    case S60CameraViewfinderEngine::OutputTypeVideoWindow:
        m_viewfinderEngine->setVideoWindowControl(output);
        break;

    default:
        break;
    }
}

void S60CameraControl::releaseVideoOutput(const S60CameraViewfinderEngine::ViewfinderOutputType type)
{
    m_viewfinderEngine->releaseControl(type);
}

void S60CameraControl::loadCamera()
{
    if (m_internalState < QCamera::LoadingStatus) {
        m_internalState = QCamera::LoadingStatus;
        emit statusChanged(m_internalState);
    } else if (m_internalState == QCamera::LoadedStatus
        || m_internalState >= QCamera::StartingStatus) {
        // Nothing to load (already loaded)
        return;
    }
    // Status = Loading or Standby

    m_cameraEngine->ReserveAndPowerOn();

    // Completion notified in MceoCameraReady()
}

void S60CameraControl::unloadCamera()
{
    if (m_internalState > QCamera::LoadingStatus) {
        m_internalState = QCamera::LoadingStatus;
        emit statusChanged(m_internalState);
    } else if (m_internalState < QCamera::LoadingStatus) {
        // Nothing to unload
        return;
    }
    // Status = Loading

    if (m_inactivityTimer->isActive())
        m_inactivityTimer->stop();

    m_cameraEngine->ReleaseAndPowerOff();

    m_internalState = QCamera::UnloadedStatus;
    emit statusChanged(m_internalState);
}

void S60CameraControl::startCamera()
{
    if (m_internalState < QCamera::StartingStatus) {
        m_internalState = QCamera::StartingStatus;
        emit statusChanged(m_internalState);
    } else if (m_internalState > QCamera::StartingStatus) {
        // Nothing to start (already started)
        return;
    }
    // Status = Starting

    if (m_inactivityTimer->isActive())
        m_inactivityTimer->stop();

    if (m_viewfinderEngine)
        m_viewfinderEngine->startViewfinder();
    else
        setError(KErrGeneral, tr("Failed to start viewfinder."));

    m_internalState = QCamera::ActiveStatus;
    emit statusChanged(m_internalState);

    emit cameraReadyChanged(true);

#ifdef Q_CC_NOKIAX86 // Emulator
    MceoCameraReady(); // Signal that we are ready
#endif
}

void S60CameraControl::stopCamera()
{
    if (m_internalState > QCamera::StartingStatus) {
        m_internalState = QCamera::StartingStatus;
        emit statusChanged(m_internalState);
    } else if (m_internalState < QCamera::StartingStatus) {
        // Nothing to stop
        return;
    }
    // Status = Starting

    // Cancel ongoing operations if any
    m_imageSession->cancelCapture();
    m_videoSession->stopRecording();

    emit cameraReadyChanged(false);
    if (m_viewfinderEngine)
        m_viewfinderEngine->stopViewfinder();
    else
        setError(KErrGeneral, tr("Failed to stop viewfinder."));

    m_internalState = QCamera::LoadedStatus;
    emit statusChanged(m_internalState);

    m_inactivityTimer->start(KInactivityTimerTimeout);
}

void S60CameraControl::videoStateChanged(const S60VideoCaptureSession::TVideoCaptureState state)
{
    // Save video state
    m_videoCaptureState = state;

    if (m_rotateCameraWhenReady) {
        if (m_videoCaptureState != S60VideoCaptureSession::ERecording &&
            m_videoCaptureState != S60VideoCaptureSession::EPaused)
            resetCameraOrientation();
    }

    // If video recording was stopped, video state reverts back to
    // Initializing. In that case revert also Camera status to notify that
    // video initialization needs to be completed.
    if (state == S60VideoCaptureSession::EInitializing) {
        if (m_internalState > QCamera::LoadingStatus) {
            m_internalState = QCamera::LoadingStatus;
            emit statusChanged(m_internalState);
        }

    // Handle video initialization completion
    } else if (state == S60VideoCaptureSession::EInitialized) {

        // Make sure state is not downgraded
        if (m_internalState == QCamera::LoadedStatus
            || m_internalState == QCamera::ActiveStatus) {
            // Do nothing (already in target state)
        } else if (m_internalState == QCamera::StartingStatus) {
            m_internalState = QCamera::ActiveStatus;
            emit statusChanged(m_internalState);
        } else {
            m_internalState = QCamera::LoadedStatus;
            emit statusChanged(m_internalState);
        }

        switch (m_requestedState) {
            case QCamera::UnloadedState:
                stopCamera();
                unloadCamera();
                if (m_changeCaptureModeWhenReady) {
                    m_settingCaptureModeInternally = true;
                    setCaptureMode(m_requestedCaptureMode);
                }
                break;
            case QCamera::LoadedState:
                stopCamera();
                if (m_changeCaptureModeWhenReady) {
                    m_settingCaptureModeInternally = true;
                    setCaptureMode(m_requestedCaptureMode);
                }
                m_inactivityTimer->start(KInactivityTimerTimeout);
                break;
            case QCamera::ActiveState:
                if (m_changeCaptureModeWhenReady) {
                    m_settingCaptureModeInternally = true;
                    setCaptureMode(m_requestedCaptureMode);
                }
                startCamera();
                break;

            default:
                setError(KErrGeneral, tr("Unexpected camera error."));
                return;
        }
    }
}

void S60CameraControl::imageCaptured(const int imageId, const QImage& preview)
{
    Q_UNUSED(imageId);
    Q_UNUSED(preview);

    // Unsubscribe the readyForCaptureChanged notification
    disconnect(m_imageSession, SIGNAL(imageCaptured(const int, const QImage&)),
        this, SLOT(imageCaptured(const int, const QImage&)));

    if (m_rotateCameraWhenReady)
        resetCameraOrientation();
}

void S60CameraControl::advancedSettingsCreated()
{
    m_advancedSettings = m_imageSession->advancedSettings();

    if (m_advancedSettings)
        connect(m_advancedSettings, SIGNAL(error(int, const QString&)), this, SIGNAL(error(int, const QString&)));
}

void S60CameraControl::MceoCameraReady()
{
    // Rotate camera if requested
    if (m_rotateCameraWhenReady) {
        resetCameraOrientation();
        return;
    }

    if (m_internalState != QCamera::LoadedStatus) {

        switch (m_requestedState) {
            case QCamera::UnloadedState:
                m_internalState = QCamera::LoadedStatus;
                emit statusChanged(QCamera::LoadedStatus);

                stopCamera();
                unloadCamera();

                if (m_changeCaptureModeWhenReady) {
                    m_settingCaptureModeInternally = true;
                    setCaptureMode(m_requestedCaptureMode);
                }
                break;

            case QCamera::LoadedState:
                if (m_captureMode == QCamera::CaptureVideo) {
                    int prepareSuccess = m_videoSession->initializeVideoRecording();
                    setError(prepareSuccess, tr("Loading video capture failed."));

                    // State change signalled when reservation is complete (in videoStateChanged())
                    return;
                }
                m_internalState = QCamera::LoadedStatus;
                emit statusChanged(QCamera::LoadedStatus);

                if (m_changeCaptureModeWhenReady) {
                    setCaptureMode(m_requestedCaptureMode);
                    m_changeCaptureModeWhenReady = false; // Reset
                }

                if (m_requestedState == QCamera::LoadedStatus &&
                    m_internalState == QCamera::LoadedStatus)
                    m_inactivityTimer->start(KInactivityTimerTimeout);
                break;

            case QCamera::ActiveState:
                if (m_captureMode == QCamera::CaptureVideo) {
                    int prepareSuccess = m_videoSession->initializeVideoRecording();
                    setError(prepareSuccess, tr("Loading video capture failed."));

                    // State change signalled when reservation is complete (in videoStateChanged())
                    return;
                }

                m_internalState = QCamera::LoadedStatus;
                emit statusChanged(QCamera::LoadedStatus);

                if (m_changeCaptureModeWhenReady) {
                    setCaptureMode(m_requestedCaptureMode);
                    m_changeCaptureModeWhenReady = false; // Reset
                }
                startCamera();
                break;

            default:
                setError(KErrGeneral, tr("Unexpected camera error."));
                return;
        }
    }
}

void S60CameraControl::MceoHandleError(TCameraEngineError aErrorType, TInt aError)
{
    Q_UNUSED(aErrorType);

    if (aError == KErrAccessDenied) {
        setError(KErrGeneral, tr("Access to camera device was rejected."));
    } else if (aError == KErrHardwareNotAvailable) {
        setError(aError, tr("Camera resources were lost."));
        toStandByStatus();
    }
    else
        setError(aError, tr("Unexpected camera error."));
}

void S60CameraControl::setError(const TInt error, const QString &description)
{
    if (error == KErrNone)
        return;

    m_error = error;
    QCamera::Error cameraError = fromSymbianErrorToQtMultimediaError(m_error);

    emit this->error(int(cameraError), description);

    // Reset everything, if other than not supported error or resource loss
    if (error != KErrNotSupported && error != KErrHardwareNotAvailable)
        resetCamera(true); // Try to recover from error
    else
        m_error = KErrNone; // Reset error
}

QCamera::Error S60CameraControl::fromSymbianErrorToQtMultimediaError(int aError)
{
    switch(aError) {
        case KErrNone:
            return QCamera::NoError; // No errors have occurred

        case KErrNotSupported:
            return QCamera::NotSupportedFeatureError; // The feature is not supported
        case KErrNotFound:
        case KErrBadHandle:
            return QCamera::ServiceMissingError; // No camera service available
        case KErrArgument:
        case KErrNotReady:
            return QCamera::InvalidRequestError; // Invalid parameter or state

        default:
            return QCamera::CameraError; // An error has occurred (i.e. General Error)
    }
}

// For S60CameraVideoDeviceControl
int S60CameraControl::deviceCount()
{
#ifdef Q_CC_NOKIAX86 // Emulator
    return 1;
#endif

    return CCameraEngine::CamerasAvailable();
}

int S60CameraControl::defaultDevice() const
{
    return KDefaultCameraDevice;
}

int S60CameraControl::selectedDevice() const
{
    return m_deviceIndex;
}

void S60CameraControl::setSelectedDevice(const int index)
{
    if (m_deviceIndex != index) {
        if (index >= 0 && index < deviceCount()) {
            m_deviceIndex = index;
            resetCamera();
        } else {
            setError(KErrNotSupported, tr("Requested camera is not available."));
        }
    }
}

QString S60CameraControl::name(const int index)
{
    QString cameraName;
    switch (index) {
        case 0:
            cameraName = tr("Primary camera");
            break;
        case 1:
            cameraName = tr("Secondary camera");
            break;
        case 2:
            cameraName = tr("Tertiary camera");
            break;

        default:
            cameraName = tr("Unidentified Camera");
            break;
    }

    return cameraName;
}

QString S60CameraControl::description(const int index)
{
    QString cameraDesc;
    switch (index) {
        case 0:
            cameraDesc = tr("Device primary camera");
            break;
        case 1:
            cameraDesc = tr("Device secondary camera");
            break;
        case 2:
            cameraDesc = tr("Device tertiary camera");
            break;

        default:
            cameraDesc = tr("Unidentified Camera");
            break;
    }

    return cameraDesc;
}

void S60CameraControl::resetCamera(bool errorHandling)
{
    if (m_inactivityTimer->isActive())
        m_inactivityTimer->stop();

    // Cancel ongoing activity
    m_imageSession->cancelCapture();
    m_videoSession->stopRecording(false); // Don't re-initialize video

    // Advanced settings must be destructed before the camera
    m_imageSession->deleteAdvancedSettings();

    // Release resources
    stopCamera();
    unloadCamera();

    disconnect(m_viewfinderEngine, SIGNAL(error(int, const QString&)), this, SIGNAL(error(int,const QString&)));
    if (m_viewfinderEngine) {
        delete m_viewfinderEngine;
        m_viewfinderEngine = 0;
    }

    if (m_cameraEngine) {
        delete m_cameraEngine;
        m_cameraEngine = 0;
    }

    TRAPD(err, m_cameraEngine = CCameraEngine::NewL(m_deviceIndex, 0, this));
    if (err) {
        m_cameraEngine = 0;
        if (errorHandling) {
            qWarning("Failed to recover from error.");
            if (err == KErrPermissionDenied)
                emit error(int(QCamera::ServiceMissingError), tr("Recovering from error failed. Possibly missing capabilities."));
            else
                emit error(int(QCamera::CameraError), tr("Recovering from error failed."));
        } else {
            if (err == KErrPermissionDenied)
                setError(err, tr("Camera device creation failed. Possibly missing capabilities."));
            else
                setError(err, tr("Camera device creation failed."));
        }
        return;
    }

    // Notify list of available camera devices has been updated
    emit devicesChanged();

    m_viewfinderEngine = new S60CameraViewfinderEngine(this, m_cameraEngine, this);
    if (m_viewfinderEngine == 0)
        setError(KErrNoMemory, tr("Viewfinder device creation failed."));
    connect(m_viewfinderEngine, SIGNAL(error(int, const QString&)), this, SIGNAL(error(int,const QString&)));

    setCameraHandles();

    // Reset state
    //setState(QCamera::UnloadedState);
    if (m_internalState != QCamera::UnloadedStatus) {
        m_internalState = QCamera::UnloadedStatus;
        emit statusChanged(m_internalState);
    }
    if (m_requestedState != QCamera::UnloadedState) {
        m_requestedState = QCamera::UnloadedState;
        emit stateChanged(m_requestedState);
    }

    // Reset error
    m_error = KErrNone;
}

/*
 * Reset everything else than viewfinder engine and errors.
 */
void S60CameraControl::resetCameraOrientation()
{
    // If camera has not been created, it will be created automatically to correct orientation
    if (!m_cameraEngine)
        return;

    // Check Image/VideoCapture allow rotation
    if ((!m_cameraEngine->IsCameraReady() && m_internalState != QCamera::UnloadedStatus) ||
        m_videoCaptureState == S60VideoCaptureSession::ERecording ||
        m_videoCaptureState == S60VideoCaptureSession::EPaused) {

        // If image capture is ongoing, request notification about the
        // completion (imageCaptured() is used because that comes asynchronously
        // after the image is captured)
        // Obs! If preview creation is changed to be synchnonously done during
        // the image capture this implementation needs to be changed)
        if (m_videoCaptureState != S60VideoCaptureSession::ERecording &&
            m_videoCaptureState != S60VideoCaptureSession::EPaused &&
            m_internalState == QCamera::ActiveStatus)
            connect(m_imageSession, SIGNAL(imageCaptured(const int, const QImage&)),
                this, SLOT(imageCaptured(const int, const QImage&)));

        m_rotateCameraWhenReady = true;
        return;
    }

    m_rotateCameraWhenReady = false; // Reset

    QCamera::State originalState = m_requestedState;

    // Cancel ongoing activity
    m_imageSession->cancelCapture();
    m_videoSession->stopRecording(false); // Don't re-initialize video

    // Advanced settings must be destructed before the camera
    m_imageSession->deleteAdvancedSettings();

    // Release resources
    stopCamera();
    unloadCamera();

    // Unset CameraEngine to ViewfinderEngine
    m_viewfinderEngine->setNewCameraEngine(0);
    if (m_cameraEngine) {
        delete m_cameraEngine;
        m_cameraEngine = 0;
    }

    TRAPD(err, m_cameraEngine = CCameraEngine::NewL(m_deviceIndex, 0, this));
    if (err) {
        setError(err, tr("Camera device creation failed."));
        return;
    }
    // Reset CameraEngine to ViewfinderEngine
    m_viewfinderEngine->setNewCameraEngine(m_cameraEngine);

    // Notify list of available camera devices has been updated
    emit devicesChanged();

    setCameraHandles();

    // Reset state
    if (m_internalState != QCamera::UnloadedStatus) {
        m_internalState = QCamera::UnloadedStatus;
        emit statusChanged(m_internalState);
    }
    if (m_requestedState != QCamera::UnloadedState) {
        m_requestedState = QCamera::UnloadedState;
        emit stateChanged(m_requestedState);
    }

    setState(originalState);
}

void S60CameraControl::setCameraHandles()
{
    m_imageSession->setCurrentDevice(m_deviceIndex);
    m_imageSession->setCameraHandle(m_cameraEngine);
    m_cameraEngine->SetImageCaptureObserver(m_imageSession);
    m_videoSession->setCameraHandle(m_cameraEngine);
}

void S60CameraControl::toStandByStatus()
{
    // Cancel ongoing operations if any
    m_imageSession->cancelCapture();
    m_videoSession->stopRecording(false); // Don't re-initialize video

    emit cameraReadyChanged(false);
    if (m_viewfinderEngine)
        m_viewfinderEngine->stopViewfinder();
    else
        setError(KErrGeneral, tr("Failed to stop viewfinder."));

    m_cameraEngine->ReleaseAndPowerOff();

    m_internalState = QCamera::StandbyStatus;
    emit statusChanged(m_internalState);
}

// End of file
