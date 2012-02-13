/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qdeclarativecamera_p.h"
#include "qdeclarativecamerapreviewprovider_p.h"

#include "qdeclarativecameraexposure_p.h"
#include "qdeclarativecameraflash_p.h"
#include "qdeclarativecamerafocus_p.h"
#include "qdeclarativecameraimageprocessing_p.h"

#include <qmediaplayercontrol.h>
#include <qmediaservice.h>
#include <qvideorenderercontrol.h>
#include <QtDeclarative/qdeclarativeinfo.h>

#include <QtCore/QTimer>
#include <QtGui/qevent.h>

QT_BEGIN_NAMESPACE

void QDeclarativeCamera::_q_error(int errorCode, const QString &errorString)
{
    emit error(Error(errorCode), errorString);
    emit errorChanged();
}

void QDeclarativeCamera::_q_updateState(QCamera::State state)
{
    emit cameraStateChanged(QDeclarativeCamera::State(state));
}

/*!
    \qmlclass Camera QDeclarativeCamera
    \brief The Camera element allows you to access viewfinder frames, and take photos and movies.
    \ingroup multimedia_qml
    \ingroup camera_qml

    \inherits Item

    This element is part of the \bold{QtMultimedia 5.0} module.

    You can use the \c Camera element to capture images and movies from a camera, and manipulate
    the capture and processing settings that get applied to the images.  To display the
    viewfinder you can use a \l VideoOutput element with the Camera element set as the source.

    \qml
    import QtQuick 2.0
    import QtMultimedia 5.0

    Camera {
        id: camera

        imageProcessing.whiteBalanceMode: CameraImageProcessing.WhiteBalanceFlash

        exposure {
            exposureCompensation: -1.0
            exposureMode: Camera.ExposurePortrait
        }

        flash.mode: Camera.FlashRedEyeReduction

        imageCapture {
            onImageCaptured: {
                photoPreview.source = preview  // Show the preview in an Image element
            }
        }
    }

    VideoOutput {
        source: camera
        focus : visible // to receive focus and capture key events when visible
    }

    Image {
        id: photoPreview
    }
    \endqml

    The various settings and functionality of the Camera stack is spread
    across a few different child properties of Camera.

    \table
    \header \o Property \o Description
    \row \o \l {CameraCapture} {imageCapture}
         \o Methods and properties for capturing still images.
    \row \o \l {CameraRecorder} {videoRecording}
         \o Methods and properties for capturing movies.
    \row \o \l {CameraExposure} {exposure}
         \o Methods and properties for adjusting exposure (aperture, shutter speed etc).
    \row \o \l {CameraFocus} {focus}
         \o Methods and properties for adjusting focus and providing feedback on autofocus progress.
    \row \o \l {CameraFlash} {flash}
         \o Methods and properties for controlling the camera flash.
    \row \o \l {CameraImageProcessing} {imageProcessing}
         \o Methods and properties for adjusting camera image processing parameters.
    \endtable

    Basic camera state management, error reporting, and simple zoom properties are
    available in the Camera element itself.  For integration with C++ code, the
    \l mediaObject property allows you to access the standard QtMultimedia camera
    controls.

    Many of the camera settings may take some time to apply, and might be limited
    to certain supported values depending on the hardware.  Several camera settings
    support both automatic and manual modes, with the current actual setting being
    used being exposed.

*/

/*!
    \class QDeclarativeCamera
    \internal
    \brief The QDeclarativeCamera class provides a camera item that you can add to a QDeclarativeView.
*/

/*!
    Construct a declarative camera object using \a parent object.
 */
QDeclarativeCamera::QDeclarativeCamera(QObject *parent) :
    QObject(parent),
    m_camera(0),
    m_pendingState(ActiveState),
    m_componentComplete(false)
{
    m_camera = new QCamera(this);

    m_imageCapture = new QDeclarativeCameraCapture(m_camera, this);
    m_videoRecorder = new QDeclarativeCameraRecorder(m_camera, this);
    m_exposure = new QDeclarativeCameraExposure(m_camera, this);
    m_flash = new QDeclarativeCameraFlash(m_camera, this);
    m_focus = new QDeclarativeCameraFocus(m_camera, this);
    m_imageProcessing = new QDeclarativeCameraImageProcessing(m_camera, this);

    connect(m_camera, SIGNAL(captureModeChanged(QCamera::CaptureModes)), this, SIGNAL(captureModeChanged()));
    connect(m_camera, SIGNAL(lockStatusChanged(QCamera::LockStatus,QCamera::LockChangeReason)), this, SIGNAL(lockStatusChanged()));
    connect(m_camera, SIGNAL(stateChanged(QCamera::State)), this, SLOT(_q_updateState(QCamera::State)));

    connect(m_camera->focus(), SIGNAL(opticalZoomChanged(qreal)), this, SIGNAL(opticalZoomChanged(qreal)));
    connect(m_camera->focus(), SIGNAL(digitalZoomChanged(qreal)), this, SIGNAL(digitalZoomChanged(qreal)));
    connect(m_camera->focus(), SIGNAL(maximumOpticalZoomChanged(qreal)), this, SIGNAL(maximumOpticalZoomChanged(qreal)));
    connect(m_camera->focus(), SIGNAL(maximumDigitalZoomChanged(qreal)), this, SIGNAL(maximumDigitalZoomChanged(qreal)));
}

/*! Destructor, clean up memory */
QDeclarativeCamera::~QDeclarativeCamera()
{
    m_camera->unload();
}

void QDeclarativeCamera::classBegin()
{
}

void QDeclarativeCamera::componentComplete()
{
    m_componentComplete = true;
    setCameraState(m_pendingState);
}

/*!
    Returns any camera error.
    \sa QDeclarativeCameraError::Error
*/
QDeclarativeCamera::Error QDeclarativeCamera::error() const
{
    return QDeclarativeCamera::Error(m_camera->error());
}

/*!
    \qmlproperty string Camera::errorString

    A description of the current error, if any.
*/
QString QDeclarativeCamera::errorString() const
{
    return m_camera->errorString();
}

/*!
    \qmlproperty enumeration Camera::captureMode

    \table
    \header \o Value \o Description
    \row \o CaptureStillImage
         \o Prepares the camera element for capturing still images.

    \row \o CaptureVideo
         \o Prepares the camera element for capturing video.

    \endtable

*/
QDeclarativeCamera::CaptureMode QDeclarativeCamera::captureMode() const
{
    return QDeclarativeCamera::CaptureMode(int(m_camera->captureMode()));
}

void QDeclarativeCamera::setCaptureMode(QDeclarativeCamera::CaptureMode mode)
{
    m_camera->setCaptureMode(QCamera::CaptureModes(int(mode)));
}


/*!
    \qmlproperty enumeration Camera::cameraState

    The current state of the camera object.

    \table
    \header \o Value \o Description
    \row \o UnloadedState
         \o The initial camera state, with the camera not loaded.
           The camera capabilities (with the exception of supported capture modes)
           are unknown. This state saves the most power, but takes the longest
           time to be ready for capture.

           While the supported settings are unknown in this state,
           you can still set the camera capture settings like codec,
           resolution, or frame rate.

    \row \o LoadedState
         \o The camera is loaded and ready to be configured.

           In the Idle state you can query camera capabilities,
           set capture resolution, codecs, and so on.

           The viewfinder is not active in the loaded state.

    \row \o ActiveState
          \o In the active state the viewfinder frames are available
             and the camera is ready for capture.
    \endtable

    The default camera state is ActiveState.
*/
QDeclarativeCamera::State QDeclarativeCamera::cameraState() const
{
    return m_componentComplete ? QDeclarativeCamera::State(m_camera->state()) : m_pendingState;
}

void QDeclarativeCamera::setCameraState(QDeclarativeCamera::State state)
{
    if (!m_componentComplete) {
        m_pendingState = state;
        return;
    }

    switch (state) {
    case QDeclarativeCamera::ActiveState:
        m_camera->start();
        break;
    case QDeclarativeCamera::UnloadedState:
        m_camera->unload();
        break;
    case QDeclarativeCamera::LoadedState:
        m_camera->load();
        break;
    }
}

/*!
    \qmlmethod Camera::start()
    \fn QDeclarativeCamera::start()

    Starts the camera.  Viewfinder frames will
    be available and image or movie capture will
    be possible.
*/
void QDeclarativeCamera::start()
{
    setCameraState(QDeclarativeCamera::ActiveState);
}

/*!
    \qmlmethod Camera::stop()
    \fn QDeclarativeCamera::stop()

    Stops the camera, but leaves the camera
    stack loaded.
*/
void QDeclarativeCamera::stop()
{
    setCameraState(QDeclarativeCamera::LoadedState);
}


/*!
    \qmlproperty enumeration Camera::lockStatus

    The overall status for all the requested camera locks.

    \table
    \header \o Value \o Description
    \row \o Unlocked
        \o The application is not interested in camera settings value.
        The camera may keep this parameter without changes, this is common with camera focus,
        or adjust exposure and white balance constantly to keep the viewfinder image nice.

    \row \o Searching
        \o The application has requested the camera focus, exposure or white balance lock with
        searchAndLock(). This state indicates the camera is focusing or calculating exposure and white balance.

    \row \o Locked
        \o The camera focus, exposure or white balance is locked.
        The camera is ready to capture, application may check the exposure parameters.

        The locked state usually means the requested parameter stays the same,
        except in the cases when the parameter is requested to be constantly updated.
        For example in continuous focusing mode, the focus is considered locked as long
        and the object is in focus, even while the actual focusing distance may be constantly changing.
    \endtable
*/
/*!
    \property QDeclarativeCamera::lockStatus

    The overall status for all the requested camera locks.

    \table
    \header \o Value \o Description
    \row \o Unlocked
        \o The application is not interested in camera settings value.
        The camera may keep this parameter without changes, this is common with camera focus,
        or adjust exposure and white balance constantly to keep the viewfinder image nice.

    \row \o Searching
        \o The application has requested the camera focus, exposure or white balance lock with
        searchAndLock(). This state indicates the camera is focusing or calculating exposure and white balance.

    \row \o Locked
        \o The camera focus, exposure or white balance is locked.
        The camera is ready to capture, application may check the exposure parameters.

        The locked state usually means the requested parameter stays the same,
        except in the cases when the parameter is requested to be constantly updated.
        For example in continuous focusing mode, the focus is considered locked as long
        and the object is in focus, even while the actual focusing distance may be constantly changing.
    \endtable
*/
QDeclarativeCamera::LockStatus QDeclarativeCamera::lockStatus() const
{
    return QDeclarativeCamera::LockStatus(m_camera->lockStatus());
}

/*!
    \qmlmethod Camera::searchAndLock()
    \fn QDeclarativeCamera::searchAndLock()

    Start focusing, exposure and white balance calculation.

    This is appropriate to call when the camera focus button is pressed
    (or on a camera capture button half-press).  If the camera supports
    autofocusing, information on the focus zones is available through
    the \l {CameraFocus}{focus} property.
*/
void QDeclarativeCamera::searchAndLock()
{
    m_camera->searchAndLock();
}

/*!
    \qmlmethod Camera::unlock()
    \fn QDeclarativeCamera::unlock()

    Unlock focus, exposure and white balance locks.
 */
void QDeclarativeCamera::unlock()
{
    m_camera->unlock();
}

/*!
    \qmlproperty real Camera::maximumOpticalZoom
    \property QDeclarativeCamera::maximumOpticalZoom

    The maximum optical zoom factor, or 1.0 if optical zoom is not supported.
*/
qreal QDeclarativeCamera::maximumOpticalZoom() const
{
    return m_camera->focus()->maximumOpticalZoom();
}

/*!
    \qmlproperty real Camera::maximumDigitalZoom
    \property  QDeclarativeCamera::maximumDigitalZoom

    The maximum digital zoom factor, or 1.0 if digital zoom is not supported.
*/
qreal QDeclarativeCamera::maximumDigitalZoom() const
{
    return m_camera->focus()->maximumDigitalZoom();
}

/*!
    \qmlproperty real Camera::opticalZoom
    \property QDeclarativeCamera::opticalZoom

    The current optical zoom factor.
*/
qreal QDeclarativeCamera::opticalZoom() const
{
    return m_camera->focus()->opticalZoom();
}

void QDeclarativeCamera::setOpticalZoom(qreal value)
{
    m_camera->focus()->zoomTo(value, digitalZoom());
}

/*!
    \qmlproperty real Camera::digitalZoom
    \property   QDeclarativeCamera::digitalZoom

    The current digital zoom factor.
*/
qreal QDeclarativeCamera::digitalZoom() const
{
    return m_camera->focus()->digitalZoom();
}

void QDeclarativeCamera::setDigitalZoom(qreal value)
{
    m_camera->focus()->zoomTo(opticalZoom(), value);
}


/*!
    \qmlsignal Camera::onError(error, errorString)

    This handler is called when an error occurs.  The enumeration value \a error is one of the
    values defined below, and a descriptive string value is available in \a errorString.

    \table
    \header \o Value \o Description
    \row \o NoError \o No errors have occurred.
    \row \o CameraError \o An error has occurred.
    \row \o InvalidRequestError \o System resource doesn't support requested functionality.
    \row \o ServiceMissingError \o No camera service available.
    \row \o NotSupportedFeatureError \o The feature is not supported.
    \endtable
*/

/*!
    \fn void QDeclarativeCamera::lockStatusChanged()

    \qmlsignal Camera::lockStatusChanged()

    This signal is emitted when the lock status (focus, exposure etc) changes.
    This can happen when locking (e.g. autofocusing) is complete or has failed.
*/

/*!
    \fn void QDeclarativeCamera::stateChanged(QDeclarativeCamera::State state)
    \qmlsignal Camera::stateChanged(state)

    This signal is emitted when the camera state has changed to \a state.  Since the
    state changes may take some time to occur this signal may arrive sometime
    after the state change has been requested.
*/

/*!
    \fn void QDeclarativeCamera:opticalZoomChanged(qreal zoom)
    \qmlsignal Camera::opticalZoomChanged(zoom)

    The optical zoom setting has changed to \a zoom.
*/

/*!
    \fn void QDeclarativeCamera::digitalZoomChanged(qreal)
    \qmlsignal Camera::digitalZoomChanged(zoom)

    The digital zoom setting has changed to \a zoom.
*/

/*!
    \fn void QDeclarativeCamera::maximumOpticalZoomChanged(zoom)
    \qmlsignal Camera::maximumOpticalZoomChanged(zoom)

    The maximum optical zoom setting has changed to \a zoom.  This
    can occur when you change between video and still image capture
    modes, or the capture settings are changed.
*/

/*!
    \fn void QDeclarativeCamera::maximumDigitalZoomChanged(qreal)
    \qmlsignal Camera::maximumDigitalZoomChanged(zoom)

    The maximum digital zoom setting has changed to \a zoom.  This
    can occur when you change between video and still image capture
    modes, or the capture settings are changed.
*/

QT_END_NAMESPACE

#include "moc_qdeclarativecamera_p.cpp"
