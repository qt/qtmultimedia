/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativecamera_p.h"
#include "qdeclarativecamerapreviewprovider_p.h"

#include "qdeclarativecameraexposure_p.h"
#include "qdeclarativecameraflash_p.h"
#include "qdeclarativetorch_p.h"
#include "qdeclarativecamerafocus_p.h"
#include "qdeclarativecameraimageprocessing_p.h"

#include "qdeclarativemediametadata_p.h"

#include <private/qplatformmediaplayer_p.h>
#include <qobject.h>
#include <QMediaDeviceManager>
#include <QtQml/qqmlinfo.h>

#include <QtCore/QTimer>
#include <QtGui/qevent.h>

QT_BEGIN_NAMESPACE

void QDeclarativeCamera::_q_errorOccurred(QCamera::Error errorCode)
{
    emit errorOccurred(Error(errorCode), errorString());
    emit errorChanged();
}

/*!
    \qmltype Camera
    \instantiates QDeclarativeCamera
    \brief Access viewfinder frames, and take photos and movies.
    \ingroup multimedia_qml
    \ingroup camera_qml
    \inqmlmodule QtMultimedia

    \inherits QtObject

    You can use \c Camera to capture images and movies from a camera, and manipulate
    the capture and processing settings that get applied to the images.  To display the
    viewfinder you can use \l VideoOutput with the Camera set as the source.

    \qml
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

            flash.mode: Camera.FlashOn

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

    If multiple cameras are available, you can select which one to use by setting the \l deviceId
    property to a value from
    \l{QtMultimedia::QtMultimedia::availableCameras}{QtMultimedia.availableCameras}.
    On a mobile device, you can conveniently switch between front-facing and back-facing cameras
    by setting the \l position property.

    The various settings and functionality of the Camera stack is spread
    across a few different child properties of Camera.

    \table
    \header \li Property \li Description
    \row \li \l {CameraCapture} {imageCapture}
         \li Methods and properties for capturing still images.
    \row \li \l {CameraRecorder} {videoRecorder}
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
    \l mediaSource property allows you to
    access the standard Qt Multimedia camera controls.

    Many of the camera settings may take some time to apply, and might be limited
    to certain supported values depending on the hardware.  Some camera settings may be
    set manually or automatically. These settings properties contain the current set value.
    For example, when autofocus is enabled the focus zones are exposed in the
    \l {CameraFocus}{focus} property.

    For additional information, read also the \l{Camera Overview}{camera overview}.
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
    m_camera(nullptr),
    m_metaData(nullptr),
    m_componentComplete(false)
{
    m_currentCameraInfo = QMediaDeviceManager::defaultVideoInput();
    m_camera = new QCamera(m_currentCameraInfo);

    m_imageCapture = new QDeclarativeCameraCapture(m_camera);
    m_videoRecorder = new QDeclarativeCameraRecorder(m_camera);
    m_exposure = new QDeclarativeCameraExposure(m_camera);
    m_flash = new QDeclarativeCameraFlash(m_camera);
    m_torch = new QDeclarativeTorch(m_camera);
    m_focus = new QDeclarativeCameraFocus(m_camera);
    m_imageProcessing = new QDeclarativeCameraImageProcessing(m_camera);

    connect(m_camera, &QCamera::activeChanged, this, &QDeclarativeCamera::activeChanged);
    connect(m_camera, SIGNAL(statusChanged(QCamera::Status)), this, SIGNAL(cameraStatusChanged()));
    connect(m_camera, SIGNAL(errorOccurred(QCamera::Error)), this, SLOT(_q_errorOccurred(QCamera::Error)));

    connect(m_camera->focus(), &QCameraFocus::zoomFactorChanged,
            this, &QDeclarativeCamera::zoomFactorChanged);
}

/*! Destructor, clean up memory */
QDeclarativeCamera::~QDeclarativeCamera()
{
    // These must be deleted before QCamera
    delete m_imageCapture;
    delete m_videoRecorder;
    delete m_exposure;
    delete m_flash;
    delete m_focus;
    delete m_imageProcessing;
    delete m_metaData;

    delete m_camera;
}

void QDeclarativeCamera::classBegin()
{
}

void QDeclarativeCamera::componentComplete()
{
    m_componentComplete = true;
    setActive(pendingActive);
}

/*!
    \qmlproperty string QtMultimedia::Camera::deviceId

    This property holds the unique identifier for the camera device being used. It may not be human-readable.

    You can get all available device IDs from \l{QtMultimedia::QtMultimedia::availableCameras}{QtMultimedia.availableCameras}.
    If no value is provided or if set to an empty string, the system's default camera will be used.

    If possible, \l cameraState, \l zoomFactor and other camera parameters are
    preserved when changing the camera device.

    \sa displayName, position
    \since 5.4
*/

QString QDeclarativeCamera::deviceId() const
{
    return QString::fromLatin1(m_currentCameraInfo.id());
}

void QDeclarativeCamera::setDeviceId(const QString &name)
{
    auto id = name.toLatin1();
    if (id == m_currentCameraInfo.id())
        return;

    setupDevice(name);
}

/*!
    \qmlproperty enumeration QtMultimedia::Camera::position

    This property holds the physical position of the camera on the hardware system.

    On a mobile device, this property can be used to easily choose between
    front-facing and back-facing cameras. If this property is set to
    \c Camera.UnspecifiedPosition, the system's default camera is used.

    If possible, \l cameraState, \l zoomFactor and other camera
    parameters are preserved when changing the camera device.

    \value  Camera.UnspecifiedPosition
            The camera position is unspecified or unknown.
    \value  Camera.BackFace
            The camera is on the back face of the system hardware. For example,
            on a mobile device, it is on side opposite from the screen.
    \value  Camera.FrontFace
            The camera is on the front face of the system hardware. For example,
            on a mobile device, it means it is on the same side as the screen.
            Viewfinder frames of front-facing cameras are mirrored horizontally,
            so the users can see themselves as looking into a mirror. Captured
            images or videos are not mirrored.

    \sa deviceId
    \since 5.4
*/

QDeclarativeCamera::Position QDeclarativeCamera::position() const
{
    return QDeclarativeCamera::Position(m_currentCameraInfo.position());
}

void QDeclarativeCamera::setPosition(Position position)
{
    QCameraInfo::Position pos = QCameraInfo::Position(position);
    if (pos == m_currentCameraInfo.position())
        return;

    QByteArray id;

    if (pos != QCameraInfo::UnspecifiedPosition) {
        const QList<QCameraInfo> cameras = QMediaDeviceManager::videoInputs();
        for (auto c : cameras) {
            if (c.position() == pos) {
                id = c.id();
                break;
            }
        }
    }
    if (id.isEmpty())
        id = QMediaDeviceManager::defaultVideoInput().id();

    if (!id.isEmpty())
        setupDevice(QString::fromLatin1(id));
}

/*!
    \qmlproperty string QtMultimedia::Camera::displayName

    This property holds the human-readable name of the camera.

    You can use this property to display the name of the camera in a user interface.

    \readonly
    \sa deviceId
    \since 5.4
*/

QString QDeclarativeCamera::displayName() const
{
    return m_currentCameraInfo.description();
}

/*!
    \qmlproperty int QtMultimedia::Camera::orientation

    This property holds the physical orientation of the camera sensor.

    The value is the orientation angle (clockwise, in steps of 90 degrees) of the camera sensor in
    relation to the display in its natural orientation.

    For example, suppose a mobile device which is naturally in portrait orientation. The back-facing
    camera is mounted in landscape. If the top side of the camera sensor is aligned with the right
    edge of the screen in natural orientation, \c orientation returns \c 270. If the top side of a
    front-facing camera sensor is aligned with the right edge of the screen, \c orientation
    returns \c 90.

    \readonly
    \sa VideoOutput::orientation
    \since 5.4
*/

void QDeclarativeCamera::setupDevice(const QString &deviceName)
{
    QCameraInfo oldCameraInfo = m_currentCameraInfo;

    auto cameras = QMediaDeviceManager::videoInputs();
    QByteArray id = deviceName.toUtf8();
    QCameraInfo info;
    for (const auto &c : cameras) {
        if (c.id() == id) {
            info = c;
            break;
        }
    }
    m_currentCameraInfo = info;
    m_camera->setCameraInfo(info);

    emit deviceIdChanged();
    if (oldCameraInfo.description() != m_currentCameraInfo.description())
        emit displayNameChanged();
    if (oldCameraInfo.position() != m_currentCameraInfo.position())
        emit positionChanged();
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
    \qmlproperty string QtMultimedia::Camera::errorString

    This property holds the last error string, if any.

    \sa errorOccurred, errorCode
*/
QString QDeclarativeCamera::errorString() const
{
    return m_camera->errorString();
}

/*!
    \qmlproperty enumeration QtMultimedia::Camera::isAvailable

    This property returns the availability of the camera.
 */
bool QDeclarativeCamera::isAvailable() const
{
    return m_camera->isAvailable();
}

/*!
    \qmlproperty bool QtMultimedia::Camera::active

    This property holds the camera object's current state. By default, the default
    camera is inactive.
*/
bool QDeclarativeCamera::isActive() const
{
    return m_componentComplete ? m_camera->isActive() : pendingActive;
}

/*!
    \qmlproperty enumeration QtMultimedia::Camera::cameraStatus

    This property holds the camera object's current status.

    \value  Camera.ActiveStatus
            The camera has been started and can produce data,
            viewfinder displays video frames.
            Depending on backend, changing camera settings such as
            capture mode, codecs, or resolution in \c {Camera.ActiveState} may
            lead to changing the status to \c LoadedStatus and \c StartingStatus
            while the settings are applied, and back to \c ActiveStatus when
            the camera is ready.
    \value  Camera.StartingStatus
            The camera is transitioning to \c {Camera.ActiveState}. The camera
            service is not ready to capture yet.
    \value  Camera.StoppingStatus
            The camera is transitioning from \c {Camera.ActiveState} to
            \c {Camera.InactiveState}.
    \value  Camera.InactiveStatus
            The camera is inactive.
    \value  Camera.UnavailableStatus
            The camera is not available.
*/
QDeclarativeCamera::Status QDeclarativeCamera::cameraStatus() const
{
    return QDeclarativeCamera::Status(m_camera->status());
}

void QDeclarativeCamera::setActive(bool active)
{
    if (!m_componentComplete) {
        pendingActive = active;
        return;
    }

    m_camera->setActive(active);
}

/*!
    \qmlmethod QtMultimedia::Camera::start()

    Starts the camera.  Viewfinder frames will
    be available and image or movie capture will
    be possible.
*/

/*!
    \qmlmethod QtMultimedia::Camera::stop()

    Stops the camera, but leaves the camera
    stack loaded.

    In this state, the camera still consumes power.
*/

/*!
    \qmlproperty real QtMultimedia::Camera::minimumZoomFactor

    This property holds the minimum zoom factor supported.
*/
qreal QDeclarativeCamera::minimumZoomFactor() const
{
    return m_camera->focus()->minimumZoomFactor();
}

/*!
    \qmlproperty real QtMultimedia::Camera::maximumZoomFactor

    This property holds the maximum zoom factor supported, or 1.0 if zooming is not supported.
*/
qreal QDeclarativeCamera::maximumZoomFactor() const
{
    return m_camera->focus()->maximumZoomFactor();
}
/*!
    \property QDeclarativeCamera::zoomFactor

    This property holds the current zoom factor.
*/
qreal QDeclarativeCamera::zoomFactor() const
{
    return m_camera->focus()->zoomFactor();
}

void QDeclarativeCamera::setZoomFactor(qreal value)
{
    m_camera->focus()->setZoomFactor(value);
}

/*!
    \qmlproperty variant QtMultimedia::Camera::mediaSource

    This property holds the native media object for the camera.

    It can be used to get a pointer to a QCamera object in order to integrate with C++ code.

    \code
        QObject *qmlCamera; // The QML Camera object
        QCamera *camera = qvariant_cast<QCamera *>(qmlCamera->property("mediaSource"));
    \endcode

    \note This property is not accessible from QML.
*/

/*!
    \qmlproperty enumeration QtMultimedia::Camera::errorCode

    This property holds the last error code.

    \value  Camera.NoError
            No errors have occurred.
    \value  Camera.CameraError
            An error has occurred.

    \sa errorOccurred, errorString
*/

/*!
    \qmlsignal QtMultimedia::Camera::error(errorCode, errorString)
    \obsolete

    Use errorOccurred() instead.
*/

/*!
    \qmlsignal QtMultimedia::Camera::errorOccurred(errorCode, errorString)
    \since 5.15

    This signal is emitted when an error specified by \a errorCode occurs.
    A descriptive string value is available in \a errorString.

    The corresponding handler is \c onError.

    \sa errorCode, errorString
*/

/*!
    \qmlsignal Camera::activeChanged(active)

    This signal is emitted when the camera state has changed to \a active. Since the
    state changes may take some time to occur this signal may arrive sometime
    after the state change has been requested.

    The corresponding handler is \c onActiveChanged.
*/

/*!
    \qmlsignal Camera::opticalZoomChanged(zoom)

    This signal is emitted when the optical zoom setting has changed to \a zoom.

    The corresponding handler is \c onOpticalZoomChanged.
*/

/*!
    \qmlsignal Camera::digitalZoomChanged(zoom)

    This signal is emitted when the digital zoom setting has changed to \a zoom.

    The corresponding handler is \c onDigitalZoomChanged.
*/

/*!
    \qmlsignal Camera::maximumOpticalZoomChanged(zoom)

    This signal is emitted when the maximum optical zoom setting has
    changed to \a zoom.  This can occur when you change between video
    and still image capture modes, or the capture settings are changed.

    The corresponding handler is \c onMaximumOpticalZoomChanged.
*/

/*!
    \qmlsignal Camera::maximumDigitalZoomChanged(zoom)

    This signal is emitted when the maximum digital zoom setting has
    changed to \a zoom.  This can occur when you change between video
    and still image capture modes, or the capture settings are changed.

    The corresponding handler is \c onMaximumDigitalZoomChanged.
*/

/*!
    \qmlpropertygroup QtMultimedia::Camera::metaData
    \qmlproperty variant QtMultimedia::Camera::metaData.cameraManufacturer
    \qmlproperty variant QtMultimedia::Camera::metaData.cameraModel
    \qmlproperty variant QtMultimedia::Camera::metaData.event
    \qmlproperty variant QtMultimedia::Camera::metaData.subject
    \qmlproperty variant QtMultimedia::Camera::metaData.orientation
    \qmlproperty variant QtMultimedia::Camera::metaData.dateTimeOriginal
    \qmlproperty variant QtMultimedia::Camera::metaData.gpsLatitude
    \qmlproperty variant QtMultimedia::Camera::metaData.gpsLongitude
    \qmlproperty variant QtMultimedia::Camera::metaData.gpsAltitude
    \qmlproperty variant QtMultimedia::Camera::metaData.gpsTimestamp
    \qmlproperty variant QtMultimedia::Camera::metaData.gpsTrack
    \qmlproperty variant QtMultimedia::Camera::metaData.gpsSpeed
    \qmlproperty variant QtMultimedia::Camera::metaData.gpsImgDirection
    \qmlproperty variant QtMultimedia::Camera::metaData.gpsProcessingMethod

    These properties hold the meta data for the camera captures.

    \list
    \li \c metaData.cameraManufacturer holds the name of the manufacturer of the camera.
    \li \c metaData.cameraModel holds the name of the model of the camera.
    \li \c metaData.event holds the event during which the photo or video is to be captured.
    \li \c metaData.subject holds the name of the subject of the capture or recording.
    \li \c metaData.orientation holds the clockwise rotation of the camera at time of capture.
    \li \c metaData.dateTimeOriginal holds the initial time at which the photo or video is captured.
    \li \c metaData.gpsLatitude holds the latitude of the camera in decimal degrees at time of capture.
    \li \c metaData.gpsLongitude holds the longitude of the camera in decimal degrees at time of capture.
    \li \c metaData.gpsAltitude holds the altitude of the camera in meters at time of capture.
    \li \c metaData.gpsTimestamp holds the timestamp of the GPS position data.
    \li \c metaData.gpsTrack holds direction of movement of the camera at the time of
           capture. It is measured in degrees clockwise from north.
    \li \c metaData.gpsSpeed holds the velocity in kilometers per hour of the camera at time of capture.
    \li \c metaData.gpsImgDirection holds direction the camera is facing at the time of capture.
           It is measured in degrees clockwise from north.
    \li \c metaData.gpsProcessingMethod holds the name of the method for determining the GPS position.
    \endlist

    \sa {QMediaMetaData}
    \since 5.4
*/

QDeclarativeMediaMetaData *QDeclarativeCamera::metaData()
{
    if (!m_metaData)
        m_metaData = new QDeclarativeMediaMetaData(m_camera);
    return m_metaData;
}

/*!
    \qmlmethod list<size> QtMultimedia::Camera::supportedViewfinderResolutions(real minimumFrameRate, real maximumFrameRate)

    Returns a list of supported viewfinder resolutions.

    If both optional parameters \a minimumFrameRate and \a maximumFrameRate are specified, the
    returned list is reduced to resolutions supported for the given frame rate range.

    The camera must be loaded before calling this function, otherwise the returned list
    is empty.

    \sa {QtMultimedia::Camera::viewfinder}{viewfinder}

    \since 5.5
*/
QJSValue QDeclarativeCamera::supportedResolutions(qreal minimumFrameRate, qreal maximumFrameRate)
{
    QQmlEngine *engine = qmlEngine(this);

    QCameraInfo info = m_camera->cameraInfo();

    const auto formats = info.videoFormats();
    QJSValue supportedResolutions = engine->newArray(formats.count());
    int i = 0;
    for (const auto &f : formats) {
        if (maximumFrameRate >= 0) {
            if (f.maxFrameRate() < minimumFrameRate ||
                f.minFrameRate() > maximumFrameRate)
                continue;
        }
        QJSValue size = engine->newObject();
        QSize resolution = f.resolution();
        size.setProperty(QStringLiteral("width"), resolution.width());
        size.setProperty(QStringLiteral("height"), resolution.height());
        supportedResolutions.setProperty(i++, size);
    }

    return supportedResolutions;
}

/*!
    \qmlmethod list<object> QtMultimedia::Camera::supportedViewfinderFrameRateRanges(size resolution)

    Returns a list of supported viewfinder frame rate ranges.

    Each range object in the list has the \c minimumFrameRate and \c maximumFrameRate properties.

    If the optional parameter \a resolution is specified, the returned list is reduced to frame rate
    ranges supported for the given \a resolution.

    The camera must be loaded before calling this function, otherwise the returned list
    is empty.

    \sa {QtMultimedia::Camera::viewfinder}{viewfinder}

    \since 5.5
*/
QJSValue QDeclarativeCamera::supportedFrameRateRanges(const QJSValue &resolution)
{
    QQmlEngine *engine = qmlEngine(this);

    QSize res;
    if (!resolution.isUndefined()) {
        QJSValue width = resolution.property(QStringLiteral("width"));
        QJSValue height = resolution.property(QStringLiteral("height"));
        if (width.isNumber() && height.isNumber())
            res = QSize(width.toInt(), height.toInt());
    }
    QCameraInfo info = m_camera->cameraInfo();

    float min = 0.;
    float max = 1.e6;
    const auto formats = info.videoFormats();
    QJSValue supportedResolutions = engine->newArray(formats.count());
    for (const auto &f : formats) {
        if (!res.isValid() || f.resolution() == res) {
            min = qMin(min, f.minFrameRate());
            max = qMin(max, f.maxFrameRate());
        }
    }

    if (max == 1.e6)
        min = max = 0.;

    // ### change Qt 5 semantics and not return an array????
    QJSValue supportedFrameRateRanges = engine->newArray(1);
    QJSValue range = engine->newObject();
    range.setProperty(QStringLiteral("minimumFrameRate"), min);
    range.setProperty(QStringLiteral("maximumFrameRate"), max);
    supportedFrameRateRanges.setProperty(0, range);

    return supportedFrameRateRanges;
}

QT_END_NAMESPACE

#include "moc_qdeclarativecamera_p.cpp"
