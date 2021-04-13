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
#include "qdeclarativecamerarecorder_p.h"

#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype CameraRecorder
    \instantiates QDeclarativeCameraRecorder
    \inqmlmodule QtMultimedia
    \brief Controls video recording with the Camera.
    \ingroup multimedia_qml
    \ingroup camera_qml

    CameraRecorder allows recording camera streams to files, and adjusting recording
    settings and metadata for videos.

    It should not be constructed separately, instead the
    \c videoRecorder property of a \l Camera should be used.

    \qml
    Camera {
        videoRecorder.audioEncodingMode: CameraRecorder.ConstantBitrateEncoding;
        videoRecorder.audioBitRate: 128000
        videoRecorder.mediaContainer: "mp4"
        // ...
    }
    \endqml

    There are many different settings for each part of the recording process (audio,
    video, and output formats), as well as control over muting and where to store
    the output file.

    \sa QAudioEncoderSettings, QVideoEncoderSettings
*/

QDeclarativeCameraRecorder::QDeclarativeCameraRecorder(QMediaCaptureSession *session, QObject *parent)
    : QObject(parent),
    m_captureSession(session)
{
    m_encoder = new QMediaEncoder(this);
    session->setEncoder(m_encoder);
    connect(m_encoder, SIGNAL(stateChanged(QMediaEncoder::State)),
            SLOT(updateRecorderState(QMediaEncoder::State)));
    connect(m_encoder, SIGNAL(statusChanged(QMediaEncoder::Status)),
            SIGNAL(recorderStatusChanged()));
    connect(m_encoder, SIGNAL(error(QMediaEncoder::Error)),
            SLOT(updateRecorderError(QMediaEncoder::Error)));
    connect(m_encoder, SIGNAL(mutedChanged(bool)), SIGNAL(mutedChanged(bool)));
    connect(m_encoder, SIGNAL(durationChanged(qint64)), SIGNAL(durationChanged(qint64)));
    connect(m_encoder, SIGNAL(actualLocationChanged(QUrl)),
            SLOT(updateActualLocation(QUrl)));
}

QDeclarativeCameraRecorder::~QDeclarativeCameraRecorder()
{
}

/*!
    \qmlproperty size QtMultimedia::CameraRecorder::resolution

    This property holds the video frame dimensions to be used for video capture.
*/
QSize QDeclarativeCameraRecorder::captureResolution()
{
    return m_encoderSettings.videoResolution();
}

/*!
    \qmlproperty string QtMultimedia::CameraRecorder::audioCodec

    This property holds the audio codec to be used for recording video.
    Typically this is \c aac or \c amr-wb.

    \sa {QtMultimedia::CameraImageProcessing::whiteBalanceMode}{whileBalanceMode}
*/
QMediaFormat::AudioCodec QDeclarativeCameraRecorder::audioCodec() const
{
    return m_encoderSettings.audioCodec();
}

/*!
    \qmlproperty string QtMultimedia::CameraRecorder::videoCodec

    This property holds the video codec to be used for recording video.
    Typically this is \c h264.
*/
QMediaFormat::VideoCodec QDeclarativeCameraRecorder::videoCodec() const
{
    return m_encoderSettings.videoCodec();
}

/*!
    \qmlproperty string QtMultimedia::CameraRecorder::mediaContainer

    This property holds the media container to be used for recording video.
    Typically this is \c mp4.
*/
QMediaFormat::FileFormat QDeclarativeCameraRecorder::mediaContainer() const
{
    return m_encoderSettings.format();
}

void QDeclarativeCameraRecorder::setCaptureResolution(const QSize &resolution)
{
    m_encoderSettings = m_encoder->encoderSettings();
    if (resolution != captureResolution()) {
        m_encoderSettings.setVideoResolution(resolution);
        m_encoder->setEncoderSettings(m_encoderSettings);
        emit captureResolutionChanged(resolution);
    }
}

void QDeclarativeCameraRecorder::setAudioCodec(QMediaFormat::AudioCodec codec)
{
    m_encoderSettings = m_encoder->encoderSettings();
    if (codec != audioCodec()) {
        m_encoderSettings.setAudioCodec(codec);
        m_encoder->setEncoderSettings(m_encoderSettings);
        emit audioCodecChanged();
    }
}

void QDeclarativeCameraRecorder::setVideoCodec(QMediaFormat::VideoCodec codec)
{
    m_encoderSettings = m_encoder->encoderSettings();
    if (codec != videoCodec()) {
        m_encoderSettings.setVideoCodec(codec);
        m_encoder->setEncoderSettings(m_encoderSettings);
        emit videoCodecChanged();
    }
}

void QDeclarativeCameraRecorder::setMediaContainer(QMediaFormat::FileFormat container)
{
    m_encoderSettings = m_encoder->encoderSettings();
    if (container != m_encoderSettings.format()) {
        m_encoderSettings.setFormat(container);
        m_encoder->setEncoderSettings(m_encoderSettings);
        emit mediaContainerChanged();
    }
}

/*!
    \qmlproperty qreal QtMultimedia::CameraRecorder::frameRate

    This property holds the framerate (in frames per second) to be used for recording video.
*/
qreal QDeclarativeCameraRecorder::frameRate() const
{
    return m_encoderSettings.videoFrameRate();
}

/*!
    \qmlproperty int QtMultimedia::CameraRecorder::videoBitRate

    This property holds the bit rate (in bits per second) to be used for recording video.
*/
int QDeclarativeCameraRecorder::videoBitRate() const
{
    return m_encoderSettings.videoBitRate();
}

/*!
    \qmlproperty int QtMultimedia::CameraRecorder::audioBitRate

    This property holds the audio bit rate (in bits per second) to be used for recording video.
*/
int QDeclarativeCameraRecorder::audioBitRate() const
{
    return m_encoderSettings.audioBitRate();
}

/*!
    \qmlproperty int QtMultimedia::CameraRecorder::audioChannels

    This property indicates the number of audio channels to be encoded while
    recording video (1 is mono, 2 is stereo).
*/
int QDeclarativeCameraRecorder::audioChannels() const
{
    return m_encoderSettings.audioChannelCount();
}

/*!
    \qmlproperty int QtMultimedia::CameraRecorder::audioSampleRate

    This property holds the sample rate to be used to encode audio while recording video.
*/
int QDeclarativeCameraRecorder::audioSampleRate() const
{
    return m_encoderSettings.audioSampleRate();
}

/*!
    \qmlproperty enumeration QtMultimedia::CameraRecorder::videoEncodingMode

    This property holds the type of encoding method to be used for recording video.

    The following are the different encoding methods used:

    \table
    \header \li Value \li Description
    \row \li ConstantQualityEncoding
         \li Encoding will aim to have a constant quality, adjusting bitrate to fit.
            This is the default.  The bitrate setting will be ignored.
    \row \li ConstantBitRateEncoding
         \li Encoding will use a constant bit rate, adjust quality to fit.  This is
            appropriate if you are trying to optimize for space.
    \row \li AverageBitRateEncoding
         \li Encoding will try to keep an average bitrate setting, but will use
            more or less as needed.
    \endtable

*/
QDeclarativeCameraRecorder::EncodingMode QDeclarativeCameraRecorder::videoEncodingMode() const
{
    return EncodingMode(m_encoderSettings.encodingMode());
}

/*!
    \qmlproperty enumeration QtMultimedia::CameraRecorder::audioEncodingMode

    The type of encoding method to use when recording audio.

    \table
    \header \li Value \li Description
    \row \li ConstantQualityEncoding
         \li Encoding will aim to have a constant quality, adjusting bitrate to fit.
            This is the default.  The bitrate setting will be ignored.
    \row \li ConstantBitRateEncoding
         \li Encoding will use a constant bit rate, adjust quality to fit.  This is
            appropriate if you are trying to optimize for space.
    \row \li AverageBitRateEncoding
         \li Encoding will try to keep an average bitrate setting, but will use
            more or less as needed.
    \endtable
*/
QDeclarativeCameraRecorder::EncodingMode QDeclarativeCameraRecorder::audioEncodingMode() const
{
    return EncodingMode(m_encoderSettings.encodingMode());
}

void QDeclarativeCameraRecorder::setFrameRate(qreal frameRate)
{
    m_encoderSettings = m_encoder->encoderSettings();
    if (!qFuzzyCompare(m_encoderSettings.videoFrameRate(),frameRate)) {
        m_encoderSettings.setVideoFrameRate(frameRate);
        m_encoder->setEncoderSettings(m_encoderSettings);
        emit frameRateChanged(frameRate);
    }
}

void QDeclarativeCameraRecorder::setVideoBitRate(int rate)
{
    m_encoderSettings = m_encoder->encoderSettings();
    if (m_encoderSettings.videoBitRate() != rate) {
        m_encoderSettings.setVideoBitRate(rate);
        m_encoder->setEncoderSettings(m_encoderSettings);
        emit videoBitRateChanged(rate);
    }
}

void QDeclarativeCameraRecorder::setAudioBitRate(int rate)
{
    m_encoderSettings = m_encoder->encoderSettings();
    if (m_encoderSettings.audioBitRate() != rate) {
        m_encoderSettings.setAudioBitRate(rate);
        m_encoder->setEncoderSettings(m_encoderSettings);
        emit audioBitRateChanged(rate);
    }
}

void QDeclarativeCameraRecorder::setAudioChannels(int channels)
{
    m_encoderSettings = m_encoder->encoderSettings();
    if (m_encoderSettings.audioChannelCount() != channels) {
        m_encoderSettings.setAudioChannelCount(channels);
        m_encoder->setEncoderSettings(m_encoderSettings);
        emit audioChannelsChanged(channels);
    }
}

void QDeclarativeCameraRecorder::setAudioSampleRate(int rate)
{
    m_encoderSettings = m_encoder->encoderSettings();
    if (m_encoderSettings.audioSampleRate() != rate) {
        m_encoderSettings.setAudioSampleRate(rate);
        m_encoder->setEncoderSettings(m_encoderSettings);
        emit audioSampleRateChanged(rate);
    }
}

void QDeclarativeCameraRecorder::setAudioEncodingMode(QDeclarativeCameraRecorder::EncodingMode encodingMode)
{
    m_encoderSettings = m_encoder->encoderSettings();
    if (m_encoderSettings.encodingMode() != QMediaEncoderSettings::EncodingMode(encodingMode)) {
        m_encoderSettings.setEncodingMode(QMediaEncoderSettings::EncodingMode(encodingMode));
        m_encoder->setEncoderSettings(m_encoderSettings);
        emit audioEncodingModeChanged(encodingMode);
    }
}

void QDeclarativeCameraRecorder::setVideoEncodingMode(QDeclarativeCameraRecorder::EncodingMode encodingMode)
{
    m_encoderSettings = m_encoder->encoderSettings();
    if (m_encoderSettings.encodingMode() != QMediaEncoderSettings::EncodingMode(encodingMode)) {
        m_encoderSettings.setEncodingMode(QMediaEncoderSettings::EncodingMode(encodingMode));
        m_encoder->setEncoderSettings(m_encoderSettings);
        emit videoEncodingModeChanged(encodingMode);
    }
}

/*!
    \qmlproperty enumeration QtMultimedia::CameraRecorder::errorCode

    This property holds the last error code.

    \table
    \header \li Value \li Description
    \row \li NoError
         \li No Errors

    \row \li ResourceError
         \li Device is not ready or not available.

    \row \li FormatError
         \li Current format is not supported.

    \row \li OutOfSpaceError
         \li No space left on device.

    \endtable
*/
QDeclarativeCameraRecorder::Error QDeclarativeCameraRecorder::errorCode() const
{
    return QDeclarativeCameraRecorder::Error(m_encoder->error());
}

/*!
    \qmlproperty string QtMultimedia::CameraRecorder::errorString

    This property holds the description of the last error.
*/
QString QDeclarativeCameraRecorder::errorString() const
{
    return m_encoder->errorString();
}

/*!
    \qmlproperty enumeration QtMultimedia::CameraRecorder::recorderState

    This property holds the current state of the camera recorder object.

    The state can be one of these two:

    \table
    \header \li Value \li Description
    \row \li StoppedState
         \li The camera is not recording video.

    \row \li RecordingState
         \li The camera is recording video.
    \endtable
*/
QDeclarativeCameraRecorder::RecorderState QDeclarativeCameraRecorder::recorderState() const
{
    //paused state is not supported for camera
    QMediaEncoder::State state = m_encoder->state();

    if (state == QMediaEncoder::PausedState)
        state = QMediaEncoder::StoppedState;

    return RecorderState(state);
}


/*!
    \qmlproperty enumeration QtMultimedia::CameraRecorder::recorderStatus

    This property holds the current status of media recording.

    \table
    \header \li Value \li Description
    \row \li UnavailableStatus
         \li Recording is not supported by the camera.
    \row \li UnloadedStatus
         \li The recorder is available but not loaded.
    \row \li LoadingStatus
         \li The recorder is initializing.
    \row \li LoadedStatus
         \li The recorder is initialized and ready to record media.
    \row \li StartingStatus
         \li Recording is requested but not active yet.
    \row \li RecordingStatus
         \li Recording is active.
    \row \li PausedStatus
         \li Recording is paused.
    \row \li FinalizingStatus
         \li Recording is stopped with media being finalized.
    \endtable
*/

QDeclarativeCameraRecorder::RecorderStatus QDeclarativeCameraRecorder::recorderStatus() const
{
    return RecorderStatus(m_encoder->status());
}

/*!
    \qmlmethod QtMultimedia::CameraRecorder::record()

    Starts recording.
*/
void QDeclarativeCameraRecorder::record()
{
    setRecorderState(RecordingState);
}

/*!
    \qmlmethod QtMultimedia::CameraRecorder::stop()

    Stops recording.
*/
void QDeclarativeCameraRecorder::stop()
{
    setRecorderState(StoppedState);
}

void QDeclarativeCameraRecorder::setRecorderState(QDeclarativeCameraRecorder::RecorderState state)
{
    if (!m_encoder)
        return;

    switch (state) {
    case QDeclarativeCameraRecorder::RecordingState:
        m_encoder->record();
        break;
    case QDeclarativeCameraRecorder::StoppedState:
        m_encoder->stop();
        break;
    }
}
/*!
    \property QDeclarativeCameraRecorder::outputLocation

    This property holds the destination location of the media content. If it is empty,
    the recorder uses the system-specific place and file naming scheme.
*/
/*!
    \qmlproperty string QtMultimedia::CameraRecorder::outputLocation

    This property holds the destination location of the media content. If the location is empty,
    the recorder uses the system-specific place and file naming scheme.
*/

QString QDeclarativeCameraRecorder::outputLocation() const
{
    return m_encoder->outputLocation().toString();
}
/*!
    \property QDeclarativeCameraRecorder::actualLocation

    This property holds the absolute location to the last saved media content.
    The location is usually available after recording starts, and reset when
    new location is set or new recording starts.
*/
/*!
    \qmlproperty string QtMultimedia::CameraRecorder::actualLocation

    This property holds the actual location of the last saved media content. The actual location is
    usually available after the recording starts, and reset when new location is set or the new recording starts.
*/

QString QDeclarativeCameraRecorder::actualLocation() const
{
    return m_encoder->actualLocation().toString();
}

void QDeclarativeCameraRecorder::setOutputLocation(const QString &location)
{
    if (outputLocation() != location) {
        m_encoder->setOutputLocation(location);
        emit outputLocationChanged(outputLocation());
    }
}
/*!
    \property QDeclarativeCameraRecorder::duration

    This property holds the duration (in miliseconds) of the last recording.
*/
/*!
    \qmlproperty int QtMultimedia::CameraRecorder::duration

   This property holds the duration (in miliseconds) of the last recording.
*/
qint64 QDeclarativeCameraRecorder::duration() const
{
    return m_encoder->duration();
}
/*!
    \property QDeclarativeCameraRecorder::muted

    This property indicates whether the audio input is muted during
    recording.
*/
/*!
    \qmlproperty bool QtMultimedia::CameraRecorder::muted

    This property indicates whether the audio input is muted during recording.
*/
bool QDeclarativeCameraRecorder::isMuted() const
{
    return m_captureSession->isMuted();
}

void QDeclarativeCameraRecorder::setMuted(bool muted)
{
    m_captureSession->setMuted(muted);
}

void QDeclarativeCameraRecorder::updateRecorderState(QMediaEncoder::State state)
{
    if (state == QMediaEncoder::PausedState)
        state = QMediaEncoder::StoppedState;

    emit recorderStateChanged(RecorderState(state));
}

void QDeclarativeCameraRecorder::updateRecorderError(QMediaEncoder::Error errorCode)
{
    qWarning() << "QMediaEncoder error:" << errorString();
    emit error(Error(errorCode), errorString());
}

void QDeclarativeCameraRecorder::updateActualLocation(const QUrl &url)
{
    emit actualLocationChanged(url.toString());
}

QT_END_NAMESPACE

#include "moc_qdeclarativecamerarecorder_p.cpp"
