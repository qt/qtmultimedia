/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
#include <QtQml/qqmlinfo.h>

#include <QtCore/QTimer>
#include <QtGui/qevent.h>

QT_BEGIN_NAMESPACE

void QDeclarativeCamera::_q_error(QCamera::Error errorCode)
{
    emit error(Error(errorCode), errorString());
    emit errorChanged();
}

void QDeclarativeCamera::_q_updateState(QCamera::State state)
{
    emit cameraStateChanged(QDeclarativeCamera::State(state));
}

void QDeclarativeCamera::_q_availabilityChanged(QMultimedia::AvailabilityStatus availability)
{
    emit availabilityChanged(Availability(availability));
}

/*!
    \qmltype Camera
    \instantiates QDeclarativeCamera
    \brief Access viewfinder frames, and take photos and movies.
    \ingroup multimedia_qml
    \ingroup camera_qml
    \inqmlmodule QtMultimedia 5.0

    \inherits Item

    Camera is part of the \b{QtMultimedia 5.0} module.

    You can use \c Camera to capture images and movies from a camera, and manipulate
    the capture and processing settings that get applied to the images.  To display the
    viewfinder you can use \l VideoOutput with the Camera set as the source.

    \qml

    import QtQuick 2.0
    import QtMultimedia 5.0

    Item {
        width: 640
        height: 360

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
                    photoPreview.source = preview  // Show the preview in an Image
                }
            }
        }

        VideoOutput {
            source: camera
            anchors.fill: parent
            focus : visible // to receive focus and capture key events when visible
        }

        Image {
            id: photoPreview
        }
    }
    \endqml

    The various settings and functionality of the Camera stack is spread
    across a few different child properties of Camera.

    \table
    \header \li Property \li Description
    \row \li \l {CameraCapture} {imageCapture}
         \li Methods and properties for capturing still images.
    \row \li \l {CameraRecorder} {videoRecording}
         \li Methods and properties for capturing movies.
    \row \li \l {CameraExposure} {exposure}
         \li Methods and properties for adjusting exposure (aperture, shutter speed etc).
    \row \li \l {CameraFocus} {focus}
         \li Methods and properties for adjusting focus and providing feedback on autofocus progress.
    \row \li \l {CameraFlash} {flash}
         \li Methods and properties for controlling the camera flash.
    \row \li \l {CameraImageProcessing} {imageProcessing}
         \li Methods and properties for adjusting camera image processing parameters.
    \endtable

    Basic camera state management, error reporting, and simple zoom properties are
    available in the Camera itself.  For integration with C++ code, the
    \l mediaObject property allows you to
    access the standard Qt Multimedia camera controls.

    Many of the camera settings may take some time to apply, and might be limited
    to certain supported values depending on the hardware.  Some camera settings may be
    set manually or automatically. These settings properties contain the current set value.
    For example, when autofocus is enabled the focus zones are exposed in the
    \l {CameraFocus}{focus} property.
*/

/*!
    \class QDeclarativeCamera
    \internal
    \brief The QDeclarativeCamera class provides a camera item that you can add to a QQuickView.
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
    connect(m_camera, SIGNAL(statusChanged(QCamera::Status)), this, SIGNAL(cameraStatusChanged()));
    connect(m_camera, SIGNAL(error(QCamera::Error)), this, SLOT(_q_error(QCamera::Error)));
    connect(m_camera, SIGNAL(availabilityChanged(QMultimedia::AvailabilityStatus)), this, SLOT(_q_availabilityChanged(QMultimedia::AvailabilityStatus)));

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
QDeclarativeCamera::Error QDeclarativeCamera::errorCode() const
{
    return QDeclarativeCamera::Error(m_camera->error());
}

/*!
    \qmlproperty string QtMultimedia5::Camera::errorString

    This property holds the last error string, if any.

    \sa QtMultimedia5::Camera::onError
*/
QString QDeclarativeCamera::errorString() const
{
    return m_camera->errorString();
}

/*!
    \qmlproperty enumeration QtMultimedia5::Camera::availability

    This property holds the availability state of the camera.

    The availability states can be one of the following:

    \table
    \header \li Value \li Description
    \row \li Available
        \li The camera is available to use
    \row \li Busy
        \li The camera is busy at the moment as it is being used by another process.
    \row \li Unavailable
        \li The camera is not available to use (there may be no camera
           hardware)
    \row \li ResourceMissing
        \li The camera cannot be used because of missing resources.
         It may be possible to try again at a later time.
    \endtable
 */
QDeclarativeCamera::Availability QDeclarativeCamera::availability() const
{
    return Availability(m_camera->availability());
}


/*!
    \qmlproperty enumeration QtMultimedia5::Camera::captureMode

    This property holds the camera capture mode, which can be one of the
    following:

    \table
    \header \li Value \li Description
    \row \li CaptureViewfinder
         \li Camera is only configured to display viewfinder.

    \row \li CaptureStillImage
         \li Prepares the Camera for capturing still images.

    \row \li CaptureVideo
         \li Prepares the Camera for capturing video.

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
    \qmlproperty enumeration QtMultimedia5::Camera::cameraState

    This property holds the camera object's current state, which can be one of the following:

    \table
    \header \li Value \li Description
    \row \li UnloadedState
         \li The initial camera state, with the camera not loaded.
           The camera capabilities (with the exception of supported capture modes)
           are unknown. This state saves the most power, but takes the longest
           time to be ready for capture.

           While the supported settings are unknown in this state,
           you can still set the camera capture settings like codec,
           resolution, or frame rate.

    \row \li LoadedState
         \li The camera is loaded and ready to be configured.

           In the Idle state you can query camera capabilities,
           set capture resolution, codecs, and so on.

           The viewfinder is not active in the loaded state.

    \row \li ActiveState
          \li In the active state the viewfinder frames are available
             and the camera is ready for capture.
    \endtable

    The default camera state is ActiveState.
*/
QDeclarativeCamera::State QDeclarativeCamera::cameraState() const
{
    return m_componentComplete ? QDeclarativeCamera::State(m_camera->state()) : m_pendingState;
}

/*!
    \qmlproperty enumeration QtMultimedia5::Camera::cameraStatus

    This property holds the camera object's current status, which can be one of the following:

    \table
    \header \li Value \li Description
    \row \li ActiveStatus
         \li The camera has been started and can produce data,
             viewfinder displays video frames.

             Depending on backend, changing camera settings such as
             capture mode, codecs, or resolution in ActiveState may lead
             to changing the status to LoadedStatus and StartingStatus while
             the settings are applied, and back to ActiveStatus when the camera is ready.

    \row \li StartingStatus
         \li The camera is starting as a result of state transition to Camera.ActiveState.
             The camera service is not ready to capture yet.

    \row \li StoppingStatus
         \li The camera is stopping as a result of state transition from Camera.ActiveState
             to Camera.LoadedState or Camera.UnloadedState.

    \row \li StandbyStatus
         \li The camera is in the power saving standby mode.
             The camera may enter standby mode after some time of inactivity
             in the Camera.LoadedState state.

    \row \li LoadedStatus
         \li The camera is loaded and ready to be configured.
             This status indicates the camera device is opened and
             it's possible to query for supported image and video capture settings
             such as resolution, frame rate, and codecs.

    \row \li LoadingStatus
         \li The camera device loading as a result of state transition from
             Camera.UnloadedState to Camera.LoadedState or Camera.ActiveState.

    \row \li UnloadingStatus
         \li The camera device is unloading as a result of state transition from
             Camera.LoadedState or Camera.ActiveState to Camera.UnloadedState.

    \row \li UnloadedStatus
         \li The initial camera status, with camera not loaded.
             The camera capabilities including supported capture settings may be unknown.

    \row \li UnavailableStatus
         \li The camera or camera backend is not available.

    \endtable
*/
QDeclarativeCamera::Status QDeclarativeCamera::cameraStatus() const
{
    return QDeclarativeCamera::Status(m_camera->status());
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
    \qmlmethod QtMultimedia5::Camera::start()

    Starts the camera.  Viewfinder frames will
    be available and image or movie capture will
    be possible.
*/
void QDeclarativeCamera::start()
{
    setCameraState(QDeclarativeCamera::ActiveState);
}

/*!
    \qmlmethod QtMultimedia5::Camera::stop()

    Stops the camera, but leaves the camera
    stack loaded.
*/
void QDeclarativeCamera::stop()
{
    setCameraState(QDeclarativeCamera::LoadedState);
}


/*!
    \qmlproperty enumeration QtMultimedia5::Camera::lockStatus

    This property holds the status of all the requested camera locks.

    The status can be one of the following values:

    \table
    \header \li Value \li Description
    \row \li Unlocked
        \li The application is not interested in camera settings value.
        The camera may keep this parameter without changes, which is common with camera focus,
        or adjust exposure and white balance constantly to keep the viewfinder image nice.

    \row \li Searching
        \li The application has requested the camera focus, exposure, or white balance lock with
        searchAndLock(). This state indicates the camera is focusing or calculating exposure and white balance.

    \row \li Locked
        \li The camera focus, exposure, or white balance is locked.
        The camera is ready to capture, and the application may check the exposure parameters.

        The locked state usually means the requested parameter stays the same,
        except in cases where the parameter is requested to be updated constantly.
        For example in continuous focusing mode, the focus is considered locked as long
        as the object is in focus, even while the actual focusing distance may be constantly changing.
    \endtable
*/
/*!
    \property QDeclarativeCamera::lockStatus

    This property holds the status of all the requested camera locks.

    The status can be one of the following:

    \table
    \header \li Value \li Description
    \row \li Unlocked
        \li The application is not interested in camera settings value.
        The camera may keep this parameter without changes, this is common with camera focus,
        or adjust exposure and white balance constantly to keep the viewfinder image nice.

    \row \li Searching
        \li The application has requested the camera focus, exposure or white balance lock with
        searchAndLock(). This state indicates the camera is focusing or calculating exposure and white balance.

    \row \li Locked
        \li The camera focus, exposure or white balance is locked.
        The camera is ready to capture, and the application may check the exposure parameters.

        The locked state usually means the requested parameter stays the same,
        except in the cases when the parameter is requested to be updated constantly.
        For example in continuous focusing mode, the focus is considered locked as long
        and the object is in focus, even while the actual focusing distance may be constantly changing.
    \endtable
*/
QDeclarativeCamera::LockStatus QDeclarativeCamera::lockStatus() const
{
    return QDeclarativeCamera::LockStatus(m_camera->lockStatus());
}

/*!
    \qmlmethod QtMultimedia5::Camera::searchAndLock()

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
    \qmlmethod QtMultimedia5::Camera::unlock()

    Unlock focus, exposure and white balance locks.
 */
void QDeclarativeCamera::unlock()
{
    m_camera->unlock();
}
/*!
    \property QDeclarativeCamera::maximumOpticalZoom

    This property holds the maximum optical zoom factor supported, or 1.0 if optical zoom is not supported.
*/
/*!
    \qmlproperty real QtMultimedia5::Camera::maximumOpticalZoom

    This property holds the maximum optical zoom factor supported, or 1.0 if optical zoom is not supported.
*/
qreal QDeclarativeCamera::maximumOpticalZoom() const
{
    return m_camera->focus()->maximumOpticalZoom();
}
/*!
    \property  QDeclarativeCamera::maximumDigitalZoom

    This property holds the maximum digital zoom factor supported, or 1.0 if digital zoom is not supported.
*/
/*!
    \qmlproperty real QtMultimedia5::Camera::maximumDigitalZoom

    This property holds the maximum digital zoom factor supported, or 1.0 if digital zoom is not supported.
*/
qreal QDeclarativeCamera::maximumDigitalZoom() const
{
    return m_camera->focus()->maximumDigitalZoom();
}
/*!
    \property QDeclarativeCamera::opticalZoom

    This property holds the current optical zoom factor.
*/

/*!
    \qmlproperty real QtMultimedia5::Camera::opticalZoom

    This property holds the current optical zoom factor.
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
    \property   QDeclarativeCamera::digitalZoom

    This property holds the current digital zoom factor.
*/
/*!
    \qmlproperty real QtMultimedia5::Camera::digitalZoom

    This property holds the current digital zoom factor.
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
    \qmlproperty variant QtMultimedia5::Camera::mediaObject

    This property holds the media object for the camera.
*/

/*!
    \qmlproperty enumeration QtMultimedia5::Camera::errorCode

    This property holds the last error code.

    \sa QtMultimedia5::Camera::onError
*/

/*!
    \qmlsignal QtMultimedia5::Camera::onError(errorCode, errorString)

    This handler is called when an error occurs. The enumeration value
    \a errorCode is one of the values defined below, and a descriptive string
    value is available in \a errorString.

    \table
    \header \li Value \li Description
    \row \li NoError \li No errors have occurred.
    \row \li CameraError \li An error has occurred.
    \row \li InvalidRequestError \li System resource doesn't support requested functionality.
    \row \li ServiceMissingError \li No camera service available.
    \row \li NotSupportedFeatureError \li The feature is not supported.
    \endtable
*/

/*!
    \qmlsignal Camera::lockStatusChanged()

    This signal is emitted when the lock status (focus, exposure etc) changes.
    This can happen when locking (e.g. autofocusing) is complete or has failed.
*/

/*!
    \qmlsignal Camera::stateChanged(state)

    This signal is emitted when the camera state has changed to \a state.  Since the
    state changes may take some time to occur this signal may arrive sometime
    after the state change has been requested.
*/

/*!
    \qmlsignal Camera::opticalZoomChanged(zoom)

    The optical zoom setting has changed to \a zoom.
*/

/*!
    \qmlsignal Camera::digitalZoomChanged(zoom)

    The digital zoom setting has changed to \a zoom.
*/

/*!
    \qmlsignal Camera::maximumOpticalZoomChanged(zoom)

    The maximum optical zoom setting has changed to \a zoom.  This
    can occur when you change between video and still image capture
    modes, or the capture settings are changed.
*/

/*!
    \qmlsignal Camera::maximumDigitalZoomChanged(zoom)

    The maximum digital zoom setting has changed to \a zoom.  This
    can occur when you change between video and still image capture
    modes, or the capture settings are changed.
*/

QT_END_NAMESPACE

#include "moc_qdeclarativecamera_p.cpp"
