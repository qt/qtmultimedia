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

#include "qmediarecorder_p.h"

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
    \class QMediaRecorder
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_recording

    \brief The QMediaRecorder class is used for the encoding and recording a capture session.

    The QMediaRecorder class is a class for encoding and recording media generated in a
    QMediaCaptureSession.

    \snippet multimedia-snippets/media.cpp Media encoder
*/

void QMediaRecorderPrivate::applySettingsLater()
{
    if (control && !settingsChanged) {
        settingsChanged = true;
        QMetaObject::invokeMethod(q_func(), "_q_applySettings", Qt::QueuedConnection);
    }
}

void QMediaRecorderPrivate::_q_applySettings()
{
    if (control && settingsChanged) {
        settingsChanged = false;
        control->applySettings();
    }
}

QString QMediaRecorderPrivate::msgFailedStartRecording()
{
    return QMediaRecorder::tr("Failed to start recording");
}

/*!
    Constructs a media encoder which records the media produced by a microphone and camera.
*/

QMediaRecorder::QMediaRecorder(QObject *parent)
    : QObject(parent),
      d_ptr(new QMediaRecorderPrivate)
{
    Q_D(QMediaRecorder);
    d->q_ptr = this;
    d->control = QPlatformMediaIntegration::instance()->createEncoder(this);
}

/*!
    Destroys a media encoder object.
*/

QMediaRecorder::~QMediaRecorder()
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
void QMediaRecorder::setCaptureSession(QMediaCaptureSession *session)
{
    Q_D(QMediaRecorder);
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
    \property QMediaRecorder::outputLocation
    \brief the destination location of media content.

    Setting the location can fail, for example when the service supports only
    local file system locations but a network URL was passed. If the service
    does not support media recording, setting the output location will
    always fail. If the operation fails an errorOccured signal is emitted.

    The \a location can be relative or empty;
    in the latter case the encoder uses the system specific place and file naming scheme.
*/

/*!
    \property QMediaRecorder::actualLocation
    \brief the actual location of the last media content.

    The actual location is usually available after recording starts,
    and reset when new location is set or new recording starts.
*/

/*!
    Returns true if media encoder service ready to use.

    \sa availabilityChanged()
*/
bool QMediaRecorder::isAvailable() const
{
    return d_func()->control != nullptr && d_func()->captureSession;
}

QUrl QMediaRecorder::outputLocation() const
{
    return d_func()->control ? d_func()->control->outputLocation() : QUrl();
}

void QMediaRecorder::setOutputLocation(const QUrl &location)
{
    Q_D(QMediaRecorder);
    if (!d->control) {
        emit errorOccurred(QMediaRecorder::ResourceError, tr("Not available"));
        return;
    }
    d->control->setOutputLocation(location);
    d->control->clearActualLocation();
    if (!location.isEmpty() && !d->control->isLocationWritable(location))
            emit errorOccurred(QMediaRecorder::LocationNotWritable, tr("Output location not writable"));
}

QUrl QMediaRecorder::actualLocation() const
{
    Q_D(const QMediaRecorder);
    return d->control ? d->control->actualLocation() : QUrl();
}

/*!
    Returns the current media encoder state.

    \sa QMediaRecorder::RecorderState
*/

QMediaRecorder::RecorderState QMediaRecorder::recorderState() const
{
    return d_func()->control ? QMediaRecorder::RecorderState(d_func()->control->state()) : StoppedState;
}

/*!
    Returns the current error state.

    \sa errorString()
*/

QMediaRecorder::Error QMediaRecorder::error() const
{
    Q_D(const QMediaRecorder);

    return d->control ? d->control->error() : QMediaRecorder::ResourceError;
}

/*!
    Returns a string describing the current error state.

    \sa error()
*/

QString QMediaRecorder::errorString() const
{
    Q_D(const QMediaRecorder);

    return d->control ? d->control->errorString() : tr("QMediaRecorder not supported on this platform");
}

/*!
    \property QMediaRecorder::duration

    \brief the recorded media duration in milliseconds.
*/

qint64 QMediaRecorder::duration() const
{
    return d_func()->control ? d_func()->control->duration() : 0;
}

/*!
    Start recording.

    While the encoder state is changed immediately to QMediaRecorder::RecordingState,
    recording may start asynchronously.

    If recording fails error() signal is emitted
    with encoder state being reset back to QMediaRecorder::StoppedState.
*/

void QMediaRecorder::record()
{
    Q_D(QMediaRecorder);

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

    The encoder state is changed to QMediaRecorder::PausedState.

    Depending on platform recording pause may be not supported,
    in this case the encoder state stays unchanged.
*/

void QMediaRecorder::pause()
{
    Q_D(QMediaRecorder);
    if (d->control && d->captureSession)
        d->control->setState(QMediaRecorder::PausedState);
}

/*!
    Stop recording.

    The encoder state is changed to QMediaRecorder::StoppedState.
*/

void QMediaRecorder::stop()
{
    Q_D(QMediaRecorder);
    if (d->control && d->captureSession)
        d->control->setState(QMediaRecorder::StoppedState);
}

/*!
    \enum QMediaRecorder::RecorderState

    \value StoppedState    The recorder is not active.
        If this is the state after recording then the actual created recording has
        finished being written to the final location and is ready on all platforms
        except on Android. On Android, due to platform limitations, there is no way
        to be certain that the recording has finished writing to the final location.
    \value RecordingState  The recording is requested.
    \value PausedState     The recorder is paused.
*/

/*!
    \enum QMediaRecorder::Error

    \value NoError         No Errors.
    \value ResourceError   Device is not ready or not available.
    \value FormatError     Current format is not supported.
    \value OutOfSpaceError No space left on device.
*/

/*!
    \property QMediaRecorder::RecorderState
    \brief The current state of the media recorder.

    The state property represents the user request and is changed synchronously
    during record(), pause() or stop() calls.
    Recorder state may also change asynchronously when recording fails.
*/

/*!
    \fn QMediaRecorder::recorderStateChanged(State state)

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
    Q_D(const QMediaRecorder);

    return d->control ? d->control->metaData() : QMediaMetaData{};
}

/*!
    Sets the meta data tp \a metaData.

    \note To ensure that meta data is set corretly, it should be set before starting the recording.
    Once the recording is stopped, any meta data set will be attached to the next recording.
*/
void QMediaRecorder::setMetaData(const QMediaMetaData &metaData)
{
    Q_D(QMediaRecorder);

    if (d->control && d->captureSession)
        d->control->setMetaData(metaData);
}

void QMediaRecorder::addMetaData(const QMediaMetaData &metaData)
{
    auto data = this->metaData();
    // merge data
    for (const auto &k : metaData.keys())
        data.insert(k, metaData.value(k));
    setMetaData(data);
}

/*!
    \fn QMediaRecorder::metaDataChanged()

    Signals that a media object's meta-data has changed.

    If multiple meta-data elements are changed,
    metaDataChanged(const QString &key, const QVariant &value) signal is emitted
    for each of them with metaDataChanged() changed emitted once.
*/

QMediaCaptureSession *QMediaRecorder::captureSession() const
{
    Q_D(const QMediaRecorder);
    return d->captureSession;
}

/*!
    \enum QMediaRecorder::EncodingQuality

    Enumerates quality encoding levels.

    \value VeryLowQuality
    \value LowQuality
    \value NormalQuality
    \value HighQuality
    \value VeryHighQuality
*/

/*!
    \enum QMediaRecorder::EncodingMode

    Enumerates encoding modes.

    \value ConstantQualityEncoding Encoding will aim to have a constant quality, adjusting bitrate to fit.
    \value ConstantBitRateEncoding Encoding will use a constant bit rate, adjust quality to fit.
    \value AverageBitRateEncoding Encoding will try to keep an average bitrate setting, but will use
           more or less as needed.
    \value TwoPassEncoding The media will first be processed to determine the characteristics,
           and then processed a second time allocating more bits to the areas
           that need it.
*/

QMediaFormat QMediaRecorder::mediaFormat() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.mediaFormat();
}

void QMediaRecorder::setMediaFormat(const QMediaFormat &format)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.mediaFormat() == format)
        return;
    d->encoderSettings.setMediaFormat(format);
    if (d->control)
        d->control->setEncoderSettings(d->encoderSettings);
    d->applySettingsLater();
    emit mediaFormatChanged();
}

/*!
    Returns the encoding mode.

    \sa EncodingMode
*/
QMediaRecorder::EncodingMode QMediaRecorder::encodingMode() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.encodingMode();
}

/*!
    Sets the encoding \a mode setting.

    If ConstantQualityEncoding is set, the quality
    encoding parameter is used and bit rates are ignored,
    otherwise the bitrates are used.

    \sa encodingMode(), EncodingMode
*/
void QMediaRecorder::setEncodingMode(EncodingMode mode)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.encodingMode() == mode)
        return;
    d->encoderSettings.setEncodingMode(mode);
    if (d->control)
        d->control->setEncoderSettings(d->encoderSettings);
    d->applySettingsLater();
    emit encodingModeChanged();
}

QMediaRecorder::Quality QMediaRecorder::quality() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.quality();
}

void QMediaRecorder::setQuality(Quality quality)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.quality() == quality)
        return;
    d->encoderSettings.setQuality(quality);
    if (d->control)
        d->control->setEncoderSettings(d->encoderSettings);
    d->applySettingsLater();
    emit qualityChanged();
}


/*!
    Returns the resolution of the encoded video.
*/
QSize QMediaRecorder::videoResolution() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.videoResolution();
}

/*!
    Sets the \a resolution of the encoded video.

    An empty QSize indicates the encoder should make an optimal choice based on
    what is available from the video source and the limitations of the codec.
*/
void QMediaRecorder::setVideoResolution(const QSize &size)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.videoResolution() == size)
        return;
    d->encoderSettings.setVideoResolution(size);
    if (d->control)
        d->control->setEncoderSettings(d->encoderSettings);
    d->applySettingsLater();
    emit videoResolutionChanged();
}

/*! \fn void QMediaRecorder::setVideoResolution(int width, int height)

    Sets the \a width and \a height of the resolution of the encoded video.

    \overload
*/

/*!
    Returns the video frame rate.
*/
qreal QMediaRecorder::videoFrameRate() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.videoFrameRate();
}

/*!
    \fn QVideoEncoderSettings::setFrameRate(qreal rate)

    Sets the video frame \a rate.

    A value of 0 indicates the encoder should make an optimal choice based on what is available
    from the video source and the limitations of the codec.
*/
void QMediaRecorder::setVideoFrameRate(qreal frameRate)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.videoFrameRate() == frameRate)
        return;
    d->encoderSettings.setVideoFrameRate(frameRate);
    if (d->control)
        d->control->setEncoderSettings(d->encoderSettings);
    d->applySettingsLater();
    emit videoFrameRateChanged();
}

/*!
    Returns the bit rate of the compressed video stream in bits per second.
*/
int QMediaRecorder::videoBitRate() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.videoBitRate();
}

/*!
    Sets the video bit \a rate in bits per second.
*/
void QMediaRecorder::setVideoBitRate(int bitRate)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.videoBitRate() == bitRate)
        return;
    d->encoderSettings.setVideoBitRate(bitRate);
    if (d->control)
        d->control->setEncoderSettings(d->encoderSettings);
    d->applySettingsLater();
    emit videoBitRateChanged();
}

/*!
    Returns the bit rate of the compressed audio stream in bits per second.
*/
int QMediaRecorder::audioBitRate() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.audioBitRate();
}

/*!
    Sets the audio bit \a rate in bits per second.
*/
void QMediaRecorder::setAudioBitRate(int bitRate)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.audioBitRate() == bitRate)
        return;
    d->encoderSettings.setAudioBitRate(bitRate);
    if (d->control)
        d->control->setEncoderSettings(d->encoderSettings);
    d->applySettingsLater();
    emit audioBitRateChanged();
}

/*!
    Returns the number of audio channels.
*/
int QMediaRecorder::audioChannelCount() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.audioChannelCount();
}

/*!
    Sets the number of audio \a channels.

    A value of -1 indicates the encoder should make an optimal choice based on
    what is available from the audio source and the limitations of the codec.
*/
void QMediaRecorder::setAudioChannelCount(int channels)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.audioChannelCount() == channels)
        return;
    d->encoderSettings.setAudioChannelCount(channels);
    if (d->control)
        d->control->setEncoderSettings(d->encoderSettings);
    d->applySettingsLater();
    emit audioChannelCountChanged();
}

/*!
    Returns the audio sample rate in Hz.
*/
int QMediaRecorder::audioSampleRate() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.audioSampleRate();
}

/*!
    Sets the audio sample \a rate in Hz.

    A value of -1 indicates the encoder should make an optimal choice based on what is avaialbe
    from the audio source and the limitations of the codec.
*/
void QMediaRecorder::setAudioSampleRate(int sampleRate)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.audioSampleRate() == sampleRate)
        return;
    d->encoderSettings.setAudioSampleRate(sampleRate);
    if (d->control)
        d->control->setEncoderSettings(d->encoderSettings);
    d->applySettingsLater();
    emit audioSampleRateChanged();
}

QT_END_NAMESPACE

#include "moc_qmediarecorder.cpp"
