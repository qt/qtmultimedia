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
#include "qdeclarativecamerarecorder_p.h"

#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

/*!
    \qmlclass CameraRecorder QDeclarativeCameraRecorder
    \inqmlmodule QtMultimedia 5
    \brief The CameraRecorder element controls video recording with the Camera.
    \ingroup multimedia_qml
    \ingroup camera_qml

    This element allows recording camera streams to files, and adjusting recording
    settings and metadata for videos.

    This element is a child of a \l Camera element (as the \c videoRecorder property)
    and cannot be created directly.

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

QDeclarativeCameraRecorder::QDeclarativeCameraRecorder(QCamera *camera, QObject *parent) :
    QObject(parent)
{
    m_recorder = new QMediaRecorder(camera, this);
    connect(m_recorder, SIGNAL(stateChanged(QMediaRecorder::State)),
            SLOT(updateRecorderState(QMediaRecorder::State)));
    connect(m_recorder, SIGNAL(statusChanged(QMediaRecorder::Status)),
            SIGNAL(recorderStatusChanged()));
    connect(m_recorder, SIGNAL(error(QMediaRecorder::Error)),
            SLOT(updateRecorderError(QMediaRecorder::Error)));
    connect(m_recorder, SIGNAL(mutedChanged(bool)), SIGNAL(mutedChanged(bool)));
    connect(m_recorder, SIGNAL(durationChanged(qint64)), SIGNAL(durationChanged(qint64)));
    connect(m_recorder, SIGNAL(actualLocationChanged(QUrl)),
            SLOT(updateActualLocation(QUrl)));
    connect(m_recorder, SIGNAL(metaDataChanged(QString,QVariant)),
            SIGNAL(metaDataChanged(QString,QVariant)));
}

QDeclarativeCameraRecorder::~QDeclarativeCameraRecorder()
{
}

/*!
    \qmlproperty size QtMultimedia5::CameraRecorder::captureResolution

    The video frame dimensions to use when capturing
    video.
*/
QSize QDeclarativeCameraRecorder::captureResolution()
{
    return m_videoSettings.resolution();
}

/*!
    \qmlproperty string QtMultimedia5::CameraRecorder::audioCodec

    The audio codec to use for recording video.
    Typically this is something like \c aac or \c amr-wb.

    \sa {QtMultimedia5::CameraImageProcessing::whiteBalanceMode}{whileBalanceMode}
*/
QString QDeclarativeCameraRecorder::audioCodec() const
{
    return m_audioSettings.codec();
}

/*!
    \qmlproperty string QtMultimedia5::CameraRecorder::videoCodec

    The video codec to use for recording video.
    Typically this is something like \c h264.
*/
QString QDeclarativeCameraRecorder::videoCodec() const
{
    return m_videoSettings.codec();
}

/*!
    \qmlproperty string QtMultimedia5::CameraRecorder::mediaContainer

    The media container to use for recording video.
    Typically this is something like \c mp4.
*/
QString QDeclarativeCameraRecorder::mediaContainer() const
{
    return m_mediaContainer;
}

void QDeclarativeCameraRecorder::setCaptureResolution(const QSize &resolution)
{
    if (resolution != captureResolution()) {
        m_videoSettings.setResolution(resolution);
        m_recorder->setVideoSettings(m_videoSettings);
        emit captureResolutionChanged(resolution);
    }
}

void QDeclarativeCameraRecorder::setAudioCodec(const QString &codec)
{
    if (codec != audioCodec()) {
        m_audioSettings.setCodec(codec);
        m_recorder->setAudioSettings(m_audioSettings);
        emit audioCodecChanged(codec);
    }
}

void QDeclarativeCameraRecorder::setVideoCodec(const QString &codec)
{
    if (codec != videoCodec()) {
        m_videoSettings.setCodec(codec);
        m_recorder->setVideoSettings(m_videoSettings);
        emit videoCodecChanged(codec);
    }
}

void QDeclarativeCameraRecorder::setMediaContainer(const QString &container)
{
    if (container != m_mediaContainer) {
        m_mediaContainer = container;
        m_recorder->setContainerFormat(container);
        emit mediaContainerChanged(container);
    }
}

/*!
    \qmlproperty qreal QtMultimedia5::CameraRecorder::frameRate

    The video framerate to use when recording video,
    in frames per second.
*/
qreal QDeclarativeCameraRecorder::frameRate() const
{
    return m_videoSettings.frameRate();
}

/*!
    \qmlproperty int QtMultimedia5::CameraRecorder::videoBitRate

    The video bit rate to use when recording video,
    in bits per second.
*/
int QDeclarativeCameraRecorder::videoBitRate() const
{
    return m_videoSettings.bitRate();
}

/*!
    \qmlproperty int QtMultimedia5::CameraRecorder::audioBitRate

    The audio bit rate to use when recording video,
    in bits per second.
*/
int QDeclarativeCameraRecorder::audioBitRate() const
{
    return m_audioSettings.bitRate();
}

/*!
    \qmlproperty int QtMultimedia5::CameraRecorder::audioChannels

    The number of audio channels to encode when
    recording video (1 is mono, 2 is stereo).
*/
int QDeclarativeCameraRecorder::audioChannels() const
{
    return m_audioSettings.channelCount();
}

/*!
    \qmlproperty int QtMultimedia5::CameraRecorder::audioSampleRate

    The audio sample rate to encode audio at, when
    recording video.
*/
int QDeclarativeCameraRecorder::audioSampleRate() const
{
    return m_audioSettings.sampleRate();
}

/*!
    \qmlproperty enumeration QtMultimedia5::CameraRecorder::videoEncodingMode

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
QDeclarativeCameraRecorder::EncodingMode QDeclarativeCameraRecorder::videoEncodingMode() const
{
    return EncodingMode(m_videoSettings.encodingMode());
}

/*!
    \qmlproperty enumeration QtMultimedia5::CameraRecorder::audioEncodingMode

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
    return EncodingMode(m_audioSettings.encodingMode());
}

void QDeclarativeCameraRecorder::setFrameRate(qreal frameRate)
{
    if (!qFuzzyCompare(m_videoSettings.frameRate(),frameRate)) {
        m_videoSettings.setFrameRate(frameRate);
        m_recorder->setVideoSettings(m_videoSettings);
        emit frameRateChanged(frameRate);
    }
}

void QDeclarativeCameraRecorder::setVideoBitRate(int rate)
{
    if (m_videoSettings.bitRate() != rate) {
        m_videoSettings.setBitRate(rate);
        m_recorder->setVideoSettings(m_videoSettings);
        emit videoBitRateChanged(rate);
    }
}

void QDeclarativeCameraRecorder::setAudioBitRate(int rate)
{
    if (m_audioSettings.bitRate() != rate) {
        m_audioSettings.setBitRate(rate);
        m_recorder->setAudioSettings(m_audioSettings);
        emit audioBitRateChanged(rate);
    }
}

void QDeclarativeCameraRecorder::setAudioChannels(int channels)
{
    if (m_audioSettings.channelCount() != channels) {
        m_audioSettings.setChannelCount(channels);
        m_recorder->setAudioSettings(m_audioSettings);
        emit audioChannelsChanged(channels);
    }
}

void QDeclarativeCameraRecorder::setAudioSampleRate(int rate)
{
    if (m_audioSettings.sampleRate() != rate) {
        m_audioSettings.setSampleRate(rate);
        m_recorder->setAudioSettings(m_audioSettings);
        emit audioSampleRateChanged(rate);
    }
}

void QDeclarativeCameraRecorder::setAudioEncodingMode(QDeclarativeCameraRecorder::EncodingMode encodingMode)
{
    if (m_audioSettings.encodingMode() != QtMultimedia::EncodingMode(encodingMode)) {
        m_audioSettings.setEncodingMode(QtMultimedia::EncodingMode(encodingMode));
        m_recorder->setAudioSettings(m_audioSettings);
        emit audioEncodingModeChanged(encodingMode);
    }
}

void QDeclarativeCameraRecorder::setVideoEncodingMode(QDeclarativeCameraRecorder::EncodingMode encodingMode)
{
    if (m_videoSettings.encodingMode() != QtMultimedia::EncodingMode(encodingMode)) {
        m_videoSettings.setEncodingMode(QtMultimedia::EncodingMode(encodingMode));
        m_recorder->setVideoSettings(m_videoSettings);
        emit videoEncodingModeChanged(encodingMode);
    }
}

// XXX todo
QMediaRecorder::Error QDeclarativeCameraRecorder::error() const
{
    return m_recorder->error();
}

/*!
    \qmlproperty string QtMultimedia5::Camera::errorString

    A description of the current error, if any.
*/
QString QDeclarativeCameraRecorder::errorString() const
{
    return m_recorder->errorString();
}

/*!
    \qmlproperty enumeration QtMultimedia5::CameraRecorder::recorderState

    The current state of the camera recorder object.

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
    QMediaRecorder::State state = m_recorder->state();

    if (state == QMediaRecorder::PausedState)
        state = QMediaRecorder::StoppedState;

    return RecorderState(state);
}


/*!
    \qmlproperty enumeration QtMultimedia5::CameraRecorder::recorderStatus

    The actual current status of media recording.

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
    return RecorderStatus(m_recorder->status());
}

/*!
    \qmlmethod QtMultimedia5::CameraRecorder::record()

    Starts recording.
*/
void QDeclarativeCameraRecorder::record()
{
    setRecorderState(RecordingState);
}

/*!
    \qmlmethod QtMultimedia5::CameraRecorder::stop()

    Stops recording.
*/
void QDeclarativeCameraRecorder::stop()
{
    setRecorderState(StoppedState);
}

void QDeclarativeCameraRecorder::setRecorderState(QDeclarativeCameraRecorder::RecorderState state)
{
    if (!m_recorder)
        return;

    switch (state) {
    case QDeclarativeCameraRecorder::RecordingState:
        m_recorder->record();
        break;
    case QDeclarativeCameraRecorder::StoppedState:
        m_recorder->stop();
        break;
    }
}

/*!
    \qmlproperty string QtMultimedia5::CameraRecorder::outputLocation
    \property QDeclarativeCameraRecorder::outputLocation

    \brief the destination location of media content.

    The location can be relative or empty;
    in this case the recorder uses the system specific place and file naming scheme.
*/

QString QDeclarativeCameraRecorder::outputLocation() const
{
    return m_recorder->outputLocation().toString();
}

/*!
    \qmlproperty string QtMultimedia5::CameraRecorder::actualLocation
    \property QDeclarativeCameraRecorder::actualLocation

    \brief the actual location of the last media content.

    The actual location is usually available after recording starts,
    and reset when new location is set or new recording starts.
*/

QString QDeclarativeCameraRecorder::actualLocation() const
{
    return m_recorder->actualLocation().toString();
}

void QDeclarativeCameraRecorder::setOutputLocation(const QString &location)
{
    if (outputLocation() != location) {
        m_recorder->setOutputLocation(location);
        emit outputLocationChanged(outputLocation());
    }
}

/*!
    \qmlproperty int QtMultimedia5::CameraRecorder::duration
    \property QDeclarativeCameraRecorder::duration

    Returns the current duration of the recording, in
    milliseconds.
*/
qint64 QDeclarativeCameraRecorder::duration() const
{
    return m_recorder->duration();
}

/*!
    \qmlproperty bool QtMultimedia5::CameraRecorder::muted
    \property QDeclarativeCameraRecorder::muted

    Whether or not the audio input is muted during
    recording.
*/
bool QDeclarativeCameraRecorder::isMuted() const
{
    return m_recorder->isMuted();
}

void QDeclarativeCameraRecorder::setMuted(bool muted)
{
    m_recorder->setMuted(muted);
}

/*!
    \qmlmethod QtMultimedia5::CameraRecorder::setMetadata(key, value)

    Sets metadata for the next video to be recorder, with
    the given \a key being associated with \a value.
*/
void QDeclarativeCameraRecorder::setMetadata(const QString &key, const QVariant &value)
{
    m_recorder->setMetaData(key, value);
}

void QDeclarativeCameraRecorder::updateRecorderState(QMediaRecorder::State state)
{
    if (state == QMediaRecorder::PausedState)
        state = QMediaRecorder::StoppedState;

    emit recorderStateChanged(RecorderState(state));
}

void QDeclarativeCameraRecorder::updateRecorderError(QMediaRecorder::Error errorCode)
{
    qWarning() << "QMediaRecorder error:" << errorString();
    emit error(errorCode);
}

void QDeclarativeCameraRecorder::updateActualLocation(const QUrl &url)
{
    emit actualLocationChanged(url.toString());
}

QT_END_NAMESPACE

#include "moc_qdeclarativecamerarecorder_p.cpp"
