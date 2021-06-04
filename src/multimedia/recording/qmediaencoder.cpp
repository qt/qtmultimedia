/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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

#include "qmediaencoder_p.h"

#include <private/qplatformmediaencoder_p.h>
#include <qaudiodevice.h>
#include <qcamera.h>
#include <qmediacapturesession.h>
#include <private/qplatformcamera_p.h>
#include <private/qplatformmediaintegration_p.h>
#include <private/qplatformmediacapture_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qurl.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qtimer.h>

#include <qaudioformat.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMediaEncoder
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_recording

    \brief The QMediaEncoder class is used for the encoding and recording a capture session.

    The QMediaEncoder class is a class for encoding and recording media generated in a
    QMediaCaptureSession.

    \snippet multimedia-snippets/media.cpp Media encoder
*/

void QMediaEncoderPrivate::applySettingsLater()
{
    if (control && !settingsChanged) {
        settingsChanged = true;
        QMetaObject::invokeMethod(q_func(), "_q_applySettings", Qt::QueuedConnection);
    }
}

void QMediaEncoderPrivate::_q_applySettings()
{
    if (control && settingsChanged) {
        settingsChanged = false;
        control->applySettings();
    }
}

/*!
    Constructs a media encoder which records the media produced by a microphone and camera.
*/

QMediaEncoder::QMediaEncoder(QObject *parent)
    : QMediaEncoderBase(parent),
      d_ptr(new QMediaEncoderPrivate)
{
    Q_D(QMediaEncoder);
    d->q_ptr = this;
    d->control = QPlatformMediaIntegration::instance()->createEncoder(this);
}

/*!
    Destroys a media encoder object.
*/

QMediaEncoder::~QMediaEncoder()
{
    if (d_ptr->captureSession) {
        if (d_ptr->captureSession->platformSession())
            d_ptr->captureSession->platformSession()->setMediaEncoder(nullptr);
        d_ptr->captureSession->setEncoder(nullptr);
    }
    delete d_ptr->control;
    delete d_ptr;
}

/*!
    \internal
*/
void QMediaEncoder::setCaptureSession(QMediaCaptureSession *session)
{
    Q_D(QMediaEncoder);
    if (d->captureSession == session)
        return;

    d->captureSession = session;

    if (!d->captureSession)
        return;

    QPlatformMediaCaptureSession *platformSession = session->platformSession();
    if (!platformSession || !d->control)
        return;

    platformSession->setMediaEncoder(d->control);
    d->applySettingsLater();

}

/*!
    \property QMediaEncoder::outputLocation
    \brief the destination location of media content.

    Setting the location can fail, for example when the service supports only
    local file system locations but a network URL was passed. If the service
    does not support media recording this setting the output location will
    always fail.

    The \a location can be relative or empty;
    in this case the encoder uses the system specific place and file naming scheme.
    After recording has stated, QMediaEncoder::outputLocation() returns the actual output location.
*/

/*!
    \property QMediaEncoder::actualLocation
    \brief the actual location of the last media content.

    The actual location is usually available after recording starts,
    and reset when new location is set or new recording starts.
*/

/*!
    Returns true if media encoder service ready to use.

    \sa availabilityChanged()
*/
bool QMediaEncoder::isAvailable() const
{
    return d_func()->control != nullptr && d_func()->captureSession;
}

QUrl QMediaEncoder::outputLocation() const
{
    return d_func()->control ? d_func()->control->outputLocation() : QUrl();
}

bool QMediaEncoder::setOutputLocation(const QUrl &location)
{
    Q_D(QMediaEncoder);
    if (!d->control)
        return false;
    d->control->clearActualLocation();
    if (d->control && d->captureSession)
        return d->control->setOutputLocation(location);
    return false;
}

QUrl QMediaEncoder::actualLocation() const
{
    Q_D(const QMediaEncoder);
    return d->control ? d->control->actualLocation() : QUrl();
}

/*!
    Returns the current media encoder state.

    \sa QMediaEncoder::State
*/

QMediaEncoder::State QMediaEncoder::state() const
{
    return d_func()->control ? QMediaEncoder::State(d_func()->control->state()) : StoppedState;
}

/*!
    Returns the current media encoder status.

    \sa QMediaEncoder::Status
*/

QMediaEncoderBase::Status QMediaEncoder::status() const
{
    Q_D(const QMediaEncoder);
    return d->control ? d->control->status() : UnavailableStatus;
}

/*!
    Returns the current error state.

    \sa errorString()
*/

QMediaEncoderBase::Error QMediaEncoder::error() const
{
    Q_D(const QMediaEncoder);

    return d->control ? d->control->error() : QMediaEncoder::ResourceError;
}

/*!
    Returns a string describing the current error state.

    \sa error()
*/

QString QMediaEncoder::errorString() const
{
    Q_D(const QMediaEncoder);

    return d->control ? d->control->errorString() : tr("QMediaEncoder not supported on this platform");
}

/*!
    \property QMediaEncoder::duration

    \brief the recorded media duration in milliseconds.
*/

qint64 QMediaEncoder::duration() const
{
    return d_func()->control ? d_func()->control->duration() : 0;
}

/*!
    Sets the encoder settings to \a settings.

    \sa QMediaEncoderSettings
*/
void QMediaEncoder::setEncoderSettings(const QMediaEncoderSettings &settings)
{
    Q_D(QMediaEncoder);

    d->encoderSettings = settings;
    if (d->control && d->captureSession)
        d->control->setEncoderSettings(settings);
    d->applySettingsLater();
}

/*!
    Returns the current encoder settings.

    \sa QMediaEncoderSettings
*/
QMediaEncoderSettings QMediaEncoder::encoderSettings() const
{
    return d_func()->encoderSettings;
}

/*!
    Start recording.

    While the encoder state is changed immediately to QMediaEncoder::RecordingState,
    recording may start asynchronously, with statusChanged(QMediaEncoder::RecordingStatus)
    signal emitted when recording starts.

    If recording fails error() signal is emitted
    with encoder state being reset back to QMediaEncoder::StoppedState.
*/

void QMediaEncoder::record()
{
    Q_D(QMediaEncoder);

    if (!d->control)
        return;
    d->control->clearActualLocation();

    if (d->settingsChanged)
        d->control->applySettings();

    d->control->clearError();

    if (d->control && d->captureSession)
        d->control->setState(QMediaRecorder::RecordingState);
}

/*!
    Pause recording.

    The encoder state is changed to QMediaEncoder::PausedState.

    Depending on platform recording pause may be not supported,
    in this case the encoder state stays unchanged.
*/

void QMediaEncoder::pause()
{
    Q_D(QMediaEncoder);
    if (d->control && d->captureSession)
        d->control->setState(QMediaRecorder::PausedState);
}

/*!
    Stop recording.

    The encoder state is changed to QMediaEncoder::StoppedState.
*/

void QMediaEncoder::stop()
{
    Q_D(QMediaEncoder);
    if (d->control && d->captureSession)
        d->control->setState(QMediaRecorder::StoppedState);
}

/*!
    \enum QMediaEncoderBase::State

    \value StoppedState    The recorder is not active.
        If this is the state after recording then the actual created recording has
        finished being written to the final location and is ready on all platforms
        except on Android. On Android, due to platform limitations, there is no way
        to be certain that the recording has finished writing to the final location.
    \value RecordingState  The recording is requested.
    \value PausedState     The recorder is paused.
*/

/*!
    \enum QMediaEncoderBase::Status

    \value UnavailableStatus
        The recorder is not available or not supported by connected media object.
    \value UnloadedStatus
        The recorder is avilable but not loaded.
    \value LoadingStatus
        The recorder is initializing.
    \value LoadedStatus
        The recorder is initialized and ready to record media.
    \value StartingStatus
        Recording is requested but not active yet.
    \value RecordingStatus
        Recording is active.
    \value PausedStatus
        Recording is paused.
    \value FinalizingStatus
        Recording is stopped with media being finalized.
*/

/*!
    \enum QMediaEncoderBase::Error

    \value NoError         No Errors.
    \value ResourceError   Device is not ready or not available.
    \value FormatError     Current format is not supported.
    \value OutOfSpaceError No space left on device.
*/

/*!
    \property QMediaEncoder::state
    \brief The current state of the media recorder.

    The state property represents the user request and is changed synchronously
    during record(), pause() or stop() calls.
    Recorder state may also change asynchronously when recording fails.
*/

/*!
    \property QMediaEncoder::status
    \brief The current status of the media recorder.

    The status is changed asynchronously and represents the actual status
    of media recorder.
*/

/*!
    \fn QMediaEncoder::stateChanged(State state)

    Signals that a media recorder's \a state has changed.
*/

/*!
    \fn QMediaEncoder::durationChanged(qint64 duration)

    Signals that the \a duration of the recorded media has changed.
*/

/*!
    \fn QMediaEncoder::actualLocationChanged(const QUrl &location)

    Signals that the actual \a location of the recorded media has changed.
    This signal is usually emitted when recording starts.
*/

/*!
    \fn QMediaEncoder::error(QMediaEncoder::Error error)

    Signals that an \a error has occurred.
*/

/*!
    \fn QMediaEncoder::availabilityChanged(bool available)

    Signals that the media recorder is now available (if \a available is true), or not.
*/

/*!
    \fn QMediaEncoder::mutedChanged(bool muted)

    Signals that the \a muted state has changed. If true the recording is being muted.
*/

/*!
    Returns the metaData associated with the recording.
*/
QMediaMetaData QMediaEncoder::metaData() const
{
    Q_D(const QMediaEncoder);

    return d->control ? d->control->metaData() : QMediaMetaData{};
}

/*!
    Sets the meta data tp \a metaData.

    \note To ensure that meta data is set corretly, it should be set before starting the recording.
    Once the recording is stopped, any meta data set will be attached to the next recording.
*/
void QMediaEncoder::setMetaData(const QMediaMetaData &metaData)
{
    Q_D(QMediaEncoder);

    if (d->control && d->captureSession)
        d->control->setMetaData(metaData);
}

void QMediaEncoder::addMetaData(const QMediaMetaData &metaData)
{
    auto data = this->metaData();
    // merge data
    for (const auto &k : metaData.keys())
        data.insert(k, metaData.value(k));
    setMetaData(data);
}

/*!
    \fn QMediaEncoder::metaDataChanged()

    Signals that a media object's meta-data has changed.

    If multiple meta-data elements are changed,
    metaDataChanged(const QString &key, const QVariant &value) signal is emitted
    for each of them with metaDataChanged() changed emitted once.
*/

QMediaCaptureSession *QMediaEncoder::captureSession() const
{
    Q_D(const QMediaEncoder);
    return d->captureSession;
}

QT_END_NAMESPACE

#include "moc_qmediaencoder.cpp"
