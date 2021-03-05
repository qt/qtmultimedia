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

#include "qmediarecorder.h"

#include <qaudiodeviceinfo.h>
#include <qcamera.h>
#include <qmediacapturesession.h>
#include <qmediaencoder.h>
#include <qcamera.h>

#include <QtCore/qdebug.h>
#include <QtCore/qurl.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class QMediaRecorderPrivate
{
    Q_DECLARE_PUBLIC(QMediaRecorder)
public:
    QMediaRecorder::CaptureMode mode = QMediaRecorder::AudioOnly;
    QMediaCaptureSession *captureSession = nullptr;
    QCamera *camera = nullptr;
    QMediaEncoder *encoder = nullptr;

    QMediaRecorder *q_ptr = nullptr;
};

/*!
    \class QMediaRecorder
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_recording

    \brief The QMediaRecorder class is used for the recording of media content.

    The QMediaRecorder class is a high level media recording class.  It's not
    intended to be used alone but for accessing the media recording functions
    of other media objects, like QCamera.

    \snippet multimedia-snippets/media.cpp Media recorder
*/

/*!
    Constructs a media recorder which records the media produced by a microphone and camera.
*/

QMediaRecorder::QMediaRecorder(QObject *parent, CaptureMode mode)
    : QMediaEncoderBase(parent),
      d_ptr(new QMediaRecorderPrivate)
{
    Q_D(QMediaRecorder);
    d->q_ptr = this;

    d->captureSession = new QMediaCaptureSession(this);
    d->encoder = new QMediaEncoder(this);
    setCaptureMode(mode);

    connect(d->encoder, &QMediaEncoder::stateChanged, this, &QMediaRecorder::stateChanged);
    connect(d->encoder, &QMediaEncoder::statusChanged, this, &QMediaRecorder::statusChanged);
    connect(d->captureSession, &QMediaCaptureSession::mutedChanged, this, &QMediaRecorder::mutedChanged);
    connect(d->captureSession, &QMediaCaptureSession::volumeChanged, this, &QMediaRecorder::volumeChanged);
    connect(d->captureSession, &QMediaCaptureSession::audioInputChanged, this, &QMediaRecorder::audioInputChanged);
}

/*!
    Destroys a media recorder object.
*/

QMediaRecorder::~QMediaRecorder()
{
    delete d_ptr;
}

/*!
    \property QMediaRecorder::outputLocation
    \brief the destination location of media content.

    Setting the location can fail, for example when the service supports only
    local file system locations but a network URL was passed. If the service
    does not support media recording this setting the output location will
    always fail.

    The \a location can be relative or empty;
    in this case the recorder uses the system specific place and file naming scheme.
    After recording has stated, QMediaRecorder::outputLocation() returns the actual output location.
*/

/*!
    \property QMediaRecorder::actualLocation
    \brief the actual location of the last media content.

    The actual location is usually available after recording starts,
    and reset when new location is set or new recording starts.
*/

/*!
    Returns true if media recorder service ready to use.

    \sa availabilityChanged()
*/
bool QMediaRecorder::isAvailable() const
{
    return d_ptr->encoder->isAvailable();
}

/*!
    \property QMediaRecorder::captureMode
    \brief The current mode the recorder operates in.

    The capture mode defines whether QMediaRecorder will record audio and
    video or audio only.

    The capture mode can only be changed while nothing is being recorded.
*/

QMediaRecorder::CaptureMode QMediaRecorder::captureMode() const
{
    return d_ptr->mode;
}

void QMediaRecorder::setCaptureMode(QMediaRecorder::CaptureMode mode)
{
    if (d_ptr->mode == mode)
        return;
    if (mode == AudioAndVideo) {
        Q_ASSERT(!d_ptr->camera);
        d_ptr->camera = new QCamera(this);
        d_ptr->captureSession->setCamera(d_ptr->camera);
    } else { // AudioOnly
        Q_ASSERT(d_ptr->camera);
        d_ptr->captureSession->setCamera(nullptr);
        delete d_ptr->camera;
        d_ptr->camera = nullptr;
    }
}

/*!
    Returns the camera object associated with this recording session.
    If the current \l captureMode is \l AudioOnly, a nullptr will be
    returned.
 */
QCamera *QMediaRecorder::camera() const
{
    return d_ptr->camera;
}

QUrl QMediaRecorder::outputLocation() const
{
    return d_ptr->encoder->outputLocation();
}

bool QMediaRecorder::setOutputLocation(const QUrl &location)
{
    Q_D(QMediaRecorder);
    return d->encoder->setOutputLocation(location);
}

/*!
    Returns the current media recorder state.

    \sa QMediaRecorder::State
*/

QMediaRecorder::State QMediaRecorder::state() const
{
    return d_ptr->encoder->state();
}

/*!
    Returns the current media recorder status.

    \sa QMediaRecorder::Status
*/

QMediaRecorder::Status QMediaRecorder::status() const
{
    return d_ptr->encoder->status();
}

/*!
    Returns the current error state.

    \sa errorString()
*/

QMediaRecorder::Error QMediaRecorder::error() const
{
    return d_ptr->encoder->error();
}

/*!
    Returns a string describing the current error state.

    \sa error()
*/

QString QMediaRecorder::errorString() const
{
    return d_ptr->encoder->errorString();
}

/*!
    \property QMediaRecorder::duration

    \brief the recorded media duration in milliseconds.
*/

qint64 QMediaRecorder::duration() const
{
    return d_ptr->encoder->duration();
}

/*!
    \property QMediaRecorder::muted

    \brief whether a recording audio stream is muted.
*/

bool QMediaRecorder::isMuted() const
{
    return d_ptr->captureSession->isMuted();
}

void QMediaRecorder::setMuted(bool muted)
{
    d_ptr->captureSession->setMuted(muted);
}

/*!
    \property QMediaRecorder::volume

    \brief the current recording audio volume.

    The volume is scaled linearly from \c 0.0 (silence) to \c 1.0 (full volume). Values outside this
    range will be clamped.

    The default volume is \c 1.0.

    UI volume controls should usually be scaled nonlinearly. For example, using a logarithmic scale
    will produce linear changes in perceived loudness, which is what a user would normally expect
    from a volume control. See QAudio::convertVolume() for more details.
*/

qreal QMediaRecorder::volume() const
{
    return d_ptr->captureSession->volume();
}

void QMediaRecorder::setVolume(qreal volume)
{
    d_ptr->captureSession->setVolume(volume);
}

/*!
    Sets the encoder settings to \a settings.

    \sa QMediaEncoderSettings
*/
void QMediaRecorder::setEncoderSettings(const QMediaEncoderSettings &settings)
{
    d_ptr->encoder->setEncoderSettings(settings);
}

/*!
    Returns the current encoder settings.

    \sa QMediaEncoderSettings
*/
QMediaEncoderSettings QMediaRecorder::encoderSettings() const
{
    return d_ptr->encoder->encoderSettings();
}

/*!
    Start recording.

    While the recorder state is changed immediately to QMediaRecorder::RecordingState,
    recording may start asynchronously, with statusChanged(QMediaRecorder::RecordingStatus)
    signal emitted when recording starts.

    If recording fails error() signal is emitted
    with recorder state being reset back to QMediaRecorder::StoppedState.
*/

void QMediaRecorder::record()
{
    d_ptr->encoder->record();
}

/*!
    Pause recording.

    The recorder state is changed to QMediaRecorder::PausedState.

    Depending on platform recording pause may be not supported,
    in this case the recorder state stays unchanged.
*/

void QMediaRecorder::pause()
{
    d_ptr->encoder->pause();
}

/*!
    Stop recording.

    The recorder state is changed to QMediaRecorder::StoppedState.
*/

void QMediaRecorder::stop()
{
    d_ptr->encoder->stop();
}


/*!
    \property QMediaRecorder::state
    \brief The current state of the media recorder.

    The state property represents the user request and is changed synchronously
    during record(), pause() or stop() calls.
    Recorder state may also change asynchronously when recording fails.
*/

/*!
    \property QMediaRecorder::status
    \brief The current status of the media recorder.

    The status is changed asynchronously and represents the actual status
    of media recorder.
*/

/*!
    \fn QMediaRecorder::stateChanged(State state)

    Signals that a media recorder's \a state has changed.
*/

/*!
    \fn QMediaRecorder::durationChanged(qint64 duration)

    Signals that the \a duration of the recorded media has changed.
*/

/*!
    \fn QMediaRecorder::actualLocationChanged(const QUrl &location)

    Signals that the actual \a location of the recorded media has changed.
    This signal is usually emitted when recording starts.
*/

/*!
    \fn QMediaRecorder::error(QMediaRecorder::Error error)

    Signals that an \a error has occurred.
*/

/*!
    \fn QMediaRecorder::availabilityChanged(bool available)

    Signals that the media recorder is now available (if \a available is true), or not.
*/

/*!
    \fn QMediaRecorder::mutedChanged(bool muted)

    Signals that the \a muted state has changed. If true the recording is being muted.
*/

/*!
    Returns the metaData associated with the recording.
*/
QMediaMetaData QMediaRecorder::metaData() const
{
    return d_ptr->encoder->metaData();
}

/*!
    Sets the meta data tp \a metaData.

    \note To ensure that meta data is set corretly, it should be set before starting the recording.
    Once the recording is stopped, any meta data set will be attached to the next recording.
*/
void QMediaRecorder::setMetaData(const QMediaMetaData &metaData)
{
    d_ptr->encoder->setMetaData(metaData);
}

void QMediaRecorder::addMetaData(const QMediaMetaData &metaData)
{
    d_ptr->encoder->addMetaData(metaData);
}

/*!
    \fn QMediaRecorder::metaDataChanged()

    Signals that a media object's meta-data has changed.

    If multiple meta-data elements are changed,
    metaDataChanged(const QString &key, const QVariant &value) signal is emitted
    for each of them with metaDataChanged() changed emitted once.
*/

/*!
    \property QMediaRecorder::audioInput
    \brief the active audio input name.

*/

/*!
    Returns the active audio input.
*/

QAudioDeviceInfo QMediaRecorder::audioInput() const
{
    return d_ptr->captureSession->audioInput();
}

QMediaCaptureSession *QMediaRecorder::captureSession() const
{
    Q_D(const QMediaRecorder);
    return d->captureSession;
}

/*!
    Set the active audio input to \a device.
*/

void QMediaRecorder::setAudioInput(const QAudioDeviceInfo &device)
{
    d_ptr->captureSession->setAudioInput(device);
}

/*!
    \fn QMediaRecorder::audioInputChanged(const QString& name)

    Signal emitted when active audio input changes to \a name.
*/

QT_END_NAMESPACE

#include "moc_qmediarecorder.cpp"
