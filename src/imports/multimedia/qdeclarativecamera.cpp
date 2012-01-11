/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
    \since 4.7
    \brief The Camera element allows you to add camera viewfinder to a scene.
    \ingroup qml-multimedia
    \inherits Item

    This element is part of the \bold{QtMultimedia 5.0} module.

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

        onImageCaptured : {
            photoPreview.source = preview  // Show the preview in an Image element
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

    You can use the \c Camera element to capture images from a camera, and manipulate the capture and
    processing settings that get applied to the image.
*/

/*!
    \class QDeclarativeCamera
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

    connect(m_camera, SIGNAL(captureModeChanged(QCamera::CaptureMode)), this, SIGNAL(captureModeChanged()));
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
    \sa QDeclarativeError::Error
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

QDeclarativeCamera::CaptureMode QDeclarativeCamera::captureMode() const
{
    return QDeclarativeCamera::CaptureMode(m_camera->captureMode());
}

void QDeclarativeCamera::setCaptureMode(QDeclarativeCamera::CaptureMode mode)
{
    m_camera->setCaptureMode(QCamera::CaptureMode(mode));
}


/*!
    \qmlproperty enumeration Camera::cameraState

    The current state of the camera object.

    \table
    \header \o Value \o Description
    \row \o UnloadedState
         \o The initial camera state, with camera not loaded,
           the camera capabilities except of supported capture modes
           are unknown.
           While the supported settings are unknown in this state,
           it's allowed to set the camera capture settings like codec,
           resolution, or frame rate.

    \row \o LoadedState
         \o The camera is loaded and ready to be configured.

           In the Idle state it's allowed to query camera capabilities,
           set capture resolution, codecs, etc.

           The viewfinder is not active in the loaded state.

    \row \o ActiveState
          \o In the active state as soon as camera is started
           the viewfinder displays video frames and the
           camera is ready for capture.
    \endtable

    The default camera state is ActiveState.
*/
/*!
    \enum QDeclarativeCamera::State
    \value UnloadedState
            The initial camera state, with camera not loaded,
            the camera capabilities except of supported capture modes
            are unknown.
            While the supported settings are unknown in this state,
            it's allowed to set the camera capture settings like codec,
            resolution, or frame rate.

    \value LoadedState
            The camera is loaded and ready to be configured.
            In the Idle state it's allowed to query camera capabilities,
            set capture resolution, codecs, etc.
            The viewfinder is not active in the loaded state.

    \value ActiveState
            In the active state as soon as camera is started
            the viewfinder displays video frames and the
            camera is ready for capture.
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

    Starts the camera.
*/
void QDeclarativeCamera::start()
{
    setCameraState(QDeclarativeCamera::ActiveState);
}

/*!
    \qmlmethod Camera::stop()
    \fn QDeclarativeCamera::stop()

    Stops the camera.
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
/*!
    \enum QDeclarativeCamera::LockStatus
    \value Unlocked
        The application is not interested in camera settings value.
        The camera may keep this parameter without changes, this is common with camera focus,
        or adjust exposure and white balance constantly to keep the viewfinder image nice.

    \value Searching
        The application has requested the camera focus, exposure or white balance lock with
        searchAndLock(). This state indicates the camera is focusing or calculating exposure and white balance.

    \value Locked
        The camera focus, exposure or white balance is locked.
        The camera is ready to capture, application may check the exposure parameters.

        The locked state usually means the requested parameter stays the same,
        except in the cases when the parameter is requested to be constantly updated.
        For example in continuous focusing mode, the focus is considered locked as long
        and the object is in focus, even while the actual focusing distance may be constantly changing.
*/
QDeclarativeCamera::LockStatus QDeclarativeCamera::lockStatus() const
{
    return QDeclarativeCamera::LockStatus(m_camera->lockStatus());
}

/*!
    \qmlmethod Camera::searchAndLock()
    \fn QDeclarativeCamera::searchAndLock()

    Start focusing, exposure and white balance calculation.
    If the camera has keyboard focus, searchAndLock() is called
    automatically when the camera focus button is pressed.
*/
void QDeclarativeCamera::searchAndLock()
{
    m_camera->searchAndLock();
}

/*!
    \qmlmethod Camera::unlock()
    \fn QDeclarativeCamera::unlock()

    Unlock focus.

    If the camera has keyboard focus, unlock() is called automatically
    when the camera focus button is released.
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
*/

/*!
    \fn void QDeclarativeCamera::stateChanged(QDeclarativeCamera::State)

    \qmlsignal Camera::stateChanged(Camera::State)
*/

/*!
    \fn void QDeclarativeCamera::imageCaptured(const QString &)

    \qmlsignal Camera::imageCaptured(string)
*/

/*!
    \fn void QDeclarativeCamera::imageSaved(const QString &)

    \qmlsignal Camera::imageSaved(string)
*/

/*!
    \fn void QDeclarativeCamera::error(QDeclarativeCamera::Error , const QString &)

    \qmlsignal Camera::error(Camera::Error, string)
*/

/*!
    \fn void QDeclarativeCamera::errorChanged()

*/
/*!
    \qmlsignal Camera::errorChanged()
*/

/*!
    \fn void QDeclarativeCamera::isoSensitivityChanged(int)
*/
/*!
    \qmlsignal Camera::isoSensitivityChanged(int)
*/

/*!
    \fn void QDeclarativeCamera::apertureChanged(qreal)

    \qmlsignal Camera::apertureChanged(real)
*/

/*!
    \fn void QDeclarativeCamera::shutterSpeedChanged(qreal)

*/
/*!
    \qmlsignal Camera::shutterSpeedChanged(real)
*/

/*!
    \fn void QDeclarativeCamera::exposureCompensationChanged(qreal)

*/
/*!
    \qmlsignal Camera::exposureCompensationChanged(real)
*/

/*!
    \fn void QDeclarativeCamera:opticalZoomChanged(qreal zoom)

    Optical zoom changed to \a zoom.
*/
/*!
    \qmlsignal Camera::opticalZoomChanged(real)
*/

/*!
    \fn void QDeclarativeCamera::digitalZoomChanged(qreal)

    \qmlsignal Camera::digitalZoomChanged(real)
*/

/*!
    \fn void QDeclarativeCamera::maximumOpticalZoomChanged(qreal)

    \qmlsignal Camera::maximumOpticalZoomChanged(real)
*/

/*!
    \fn void QDeclarativeCamera::maximumDigitalZoomChanged(qreal)

    \qmlsignal Camera::maximumDigitalZoomChanged(real)
*/


/*!
    \fn void QDeclarativeCamera::captureResolutionChanged(const QSize &)

    \qmlsignal Camera::captureResolutionChanged(Item)
*/

/*!
    \fn QDeclarativeCamera::cameraStateChanged(QDeclarativeCamera::State)

*/


QT_END_NAMESPACE

#include "moc_qdeclarativecamera_p.cpp"
