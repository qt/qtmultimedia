// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediarecorder_p.h"

#include <private/qplatformmediarecorder_p.h>
#include <qaudiodevice.h>
#include <qcamera.h>
#include <qscreencapture.h>
#include <qwindowcapture.h>
#include <qmediacapturesession.h>
#include <private/qplatformcamera_p.h>
#include <private/qplatformsurfacecapture_p.h>
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
    \ingroup multimedia_video
    \ingroup multimedia_audio

    \brief The QMediaRecorder class is used for encoding and recording a capture session.

    Use the QMediaRecorder class to encode and record media generated in
    \l QMediaCaptureSession. You can generate:
    \list
        \li Audio. Use \l QAudioInput or \l QAudioBufferInput.
        \li Video. Use \l QCamera, \l QScreenCapture, \l QWindowCapture, or \l QVideoFrameInput.
    \endlist

    To record media, connect a generator to a corresponding media capture session.

    Performance of video encoding and recording is limited by the hardware,
    the operating system, the installed graphic drivers, and the input video format.
    If \c QCamera, \c QScreenCapture, or \c QWindowCapture produces video frames
    faster than \c QMediaRecorder can encode and record them, the recorder
    may drop some frames. This is likely to occur if the input frame resolution
    is high, 4K for example, and hardware-accelerated encoding is unavailable.
    If you generate input video via \c QVideoFrameInput, the method
    \c QVideoFrameInput::sendVideoFrame will do nothing and return \c false
    whenever this limitation is reached and the internal frame queue is full.
    Rely on the signal \c QVideoFrameInput::readyToSendVideoFrame to know
    when the recorder is ready to receive new frames again.
    If you cannot change the rate of video frame generation and dropping frames
    is undesirable, we recommend implementing your own frame queue on top of
    \c QVideoFrameInput, considering the memory limitations of the hardware.

    \snippet multimedia-snippets/media.cpp Media recorder
*/
/*!
    \qmltype MediaRecorder
    \nativetype QMediaRecorder
    \brief For encoding and recording media generated in a CaptureSession.

    \inqmlmodule QtMultimedia
    \ingroup multimedia_qml
    \ingroup multimedia_audio_qml
    \ingroup multimedia_video_qml

    Use the MediaRecorder element within a CaptureSession to encode and record:
    \list
        \li Audio captured from an audio interface (like microphone or line input).
        \li Video captured from camera, screen, or an application window.
    \endlist

    Performance of video encoding and recording is limited by the hardware,
    the operating system, the installed graphic drivers, and the input video format.
    If \c Camera, \c ScreenCapture, or \c WindowCapture produces video frames
    faster than \c MediaRecorder can encode and record them, the recorder
    may drop some frames. This is likely to occur if the input frame resolution
    is high, 4K for example, and hardware-accelerated encoding is unavailable.

    \since 6.2
    The code below shows a simple capture session containing a MediaRecorder using the default
    camera and default audio input.

\qml
    CaptureSession {
        id: captureSession
        camera: Camera {
            id: camera
            active: true
        }
        audioInput: AudioInput {}
        recorder: MediaRecorder {
            id: recorder
        }
    }
\endqml

    The code below shows how the recording can be started and stopped.
\qml
    CameraButton {
        text: "Record"
        visible: recorder.recorderState !== MediaRecorder.RecordingState
        onClicked: recorder.record()
    }

    CameraButton {
        id: stopButton
        text: "Stop"
        visible: recorder.recorderState === MediaRecorder.RecordingState
        onClicked: recorder.stop()
    }
\endqml

    \sa CaptureSession, Camera, ScreenCapture, WindowCapture, AudioInput, ImageCapture
*/
QMediaRecorderPrivate::QMediaRecorderPrivate()
{
#if QT_CONFIG(mimetype)
    // Force an early initialization of the mime database
    // to avoid a delay when recording for the first time.
    encoderSettings.mimeType();
#endif
}

QString QMediaRecorderPrivate::msgFailedStartRecording()
{
    return QMediaRecorder::tr("Failed to start recording");
}

/*!
    Constructs a media recorder.
    The media recorder is a child of \a{parent}.
*/

QMediaRecorder::QMediaRecorder(QObject *parent)
    : QObject(parent),
      d_ptr(new QMediaRecorderPrivate)
{
    Q_D(QMediaRecorder);

    auto &mediaIntegration = *QPlatformMediaIntegration::instance();

    d->q_ptr = this;
    auto maybeControl = mediaIntegration.createRecorder(this);
    if (maybeControl) {
        // The first format info initialization may take some time,
        // for users it seems to be more suitable to have a delay on the object construction
        // rather than on QMediaRecorder::record
        mediaIntegration.formatInfo();

        d->control = maybeControl.value();
    } else {
        d->initErrorMessage = maybeControl.error();
        qWarning() << "Failed to initialize QMediaRecorder" << maybeControl.error();
    }
}

/*!
    Destroys a media recorder object.
*/

QMediaRecorder::~QMediaRecorder()
{
    if (d_ptr->captureSession)
        d_ptr->captureSession->setRecorder(nullptr);
    delete d_ptr->control;
    delete d_ptr;
}

/*!
    \internal
*/
QPlatformMediaRecorder *QMediaRecorder::platformRecoder() const
{
    return d_ptr->control;
}

/*!
    \internal
*/
void QMediaRecorder::setCaptureSession(QMediaCaptureSession *session)
{
    Q_D(QMediaRecorder);
    d->captureSession = session;
}
/*!
    \qmlproperty url QtMultimedia::MediaRecorder::outputLocation
    \brief The destination location of media content.

    Setting the location can fail, for example when the service supports only
    local file system locations but a network URL was passed. If the operation
    fails, the \l errorOccurred() signal is emitted.

    The output location can be empty, a directory, or a file. The path to a
    directory or file can be relative or absolute. The \l record() method
    generates the actual location according to the specified output location and
    system-specific settings. Refer to the \l actualLocation property description
    for details.

   \sa actualLocation, errorOccurred()
*/

/*!
    \property QMediaRecorder::outputLocation
    \brief The destination location of media content.

    Setting the location can fail, for example when the service supports only
    local file system locations but a network URL was passed. If the operation
    fails, the \l errorOccurred() signal is emitted.

    The output location is ignored if a writable \l outputDevice
    has been assigned to the recorder.
    This behavior may change in the future, so we recommend setting only one output,
    either \c outputLocation or \c outputDevice.

    The output location can be empty, a directory, or a file. The path to a
    directory or file can be relative or absolute. The \l record() method
    generates the actual location according to the specified output location and
    system-specific settings. Refer to the \l actualLocation property description
    for details.

    \sa actualLocation, outputDevice()
*/

/*!
    \qmlproperty url QtMultimedia::MediaRecorder::actualLocation
    \brief The actual location of the last media content.

    The actual location is reset when a new \l outputLocation is assigned.
    When \l record() is invoked, the recorder generates the actual location
    basing on the following rules.
    \list
        \li If \c outputLocation is empty, a directory, or a file
            without an extension, the recorder generates the appropriate extension
            based on the selected media format and system MIME types.
        \li If \c outputLocation is a directory, the recorder generates a new file
            name within it.
        \li If \c outputLocation is empty, the recorder generates a new file name in
            the system-specific directory for audio or video.
        \li The recorder generates the actual location before
            emitting \c recorderStateChanged(RecordingState).
    \endlist
*/

/*!
    \property QMediaRecorder::actualLocation
    \brief The actual location of the last media content.

    The actual location is reset when a new \l outputLocation
    or a non-null \l outputDevice is assigned.
    When \l record() is invoked and \c outputDevice is \c null or not writable,
    the recorder generates the actual location basing on the following rules.
    \list
        \li If \c outputLocation is empty, a directory, or a file
            without an extension, the recorder generates the appropriate extension
            based on the selected media format and system MIME types.
        \li If \c outputLocation is a directory, the recorder generates a new file
            name within it.
        \li If \c outputLocation is empty, the recorder generates a new file name in
            the system-specific directory for audio or video.
        \li The recorder generates the actual location before
            emitting \c recorderStateChanged(RecordingState).
    \endlist
*/

/*!
    Returns \c true if media recorder service ready to use.
*/
bool QMediaRecorder::isAvailable() const
{
    return d_func()->control && d_func()->captureSession;
}

QUrl QMediaRecorder::outputLocation() const
{
    return d_func()->control ? d_func()->control->outputLocation() : QUrl();
}

void QMediaRecorder::setOutputLocation(const QUrl &location)
{
    Q_D(QMediaRecorder);
    if (!d->control) {
        emit errorOccurred(QMediaRecorder::ResourceError, d->initErrorMessage);
        return;
    }
    d->control->setOutputLocation(location);
    d->control->clearActualLocation();
    if (!location.isEmpty() && !d->control->isLocationWritable(location))
        emit errorOccurred(QMediaRecorder::LocationNotWritable,
                           QStringLiteral("Output location not writable"));
}

/*!
    Set the output IO device for media content.

    The \a device must have been opened in the \l{QIODevice::WriteOnly}{WriteOnly} or
    \l{QIODevice::ReadWrite}{ReadWrite} modes before the recording starts.

    The media recorder doesn't take ownership of the specified \a device.
    If the recording has been started, the device must be kept alive and open until
    the signal \c recorderStateChanged(StoppedState) is emitted.

    This method resets \l actualLocation immediately unless
    the specified \a device is \c null.

    If a writable output device is assigned to the recorder, \l outputLocation
    is ignored, and \l actualLocation is not generated when recording starts.
    This behavior may change in the future, so we recommend setting only
    one output, either \c outputLocation or \c outputDevice.

    \c QMediaRecorder::setOutputDevice is only supported with the FFmpeg backend.

    \sa outputDevice(), outputLocation
*/
void QMediaRecorder::setOutputDevice(QIODevice *device)
{
    Q_D(QMediaRecorder);
    if (!d->control) {
        emit errorOccurred(QMediaRecorder::ResourceError, d->initErrorMessage);
        return;
    }

    d->control->setOutputDevice(device);

    if (device)
        d->control->clearActualLocation();
}

/*!
    Returns the output IO device for media content.

    \sa setOutputDevice()
*/
QIODevice *QMediaRecorder::outputDevice() const
{
    Q_D(const QMediaRecorder);
    return d->control ? d->control->outputDevice() : nullptr;
}

QUrl QMediaRecorder::actualLocation() const
{
    Q_D(const QMediaRecorder);
    return d->control ? d->control->actualLocation() : QUrl();
}

/*!
    Returns the current media recorder state.

    \sa QMediaRecorder::RecorderState
*/

QMediaRecorder::RecorderState QMediaRecorder::recorderState() const
{
    return d_func()->control ? QMediaRecorder::RecorderState(d_func()->control->state()) : StoppedState;
}

/*!
    \property QMediaRecorder::error

    Returns the current error state.

    \sa errorString()
*/

QMediaRecorder::Error QMediaRecorder::error() const
{
    Q_D(const QMediaRecorder);

    return d->control ? d->control->error() : QMediaRecorder::ResourceError;
}
/*!
    \qmlproperty string QtMultimedia::MediaRecorder::errorString
    \brief This property holds a string describing the current error state.

    \sa error
*/
/*!
    \property QMediaRecorder::errorString

    Returns a string describing the current error state.

    \sa error()
*/

QString QMediaRecorder::errorString() const
{
    Q_D(const QMediaRecorder);

    return d->control ? d->control->errorString() : d->initErrorMessage;
}
/*!
    \qmlproperty qint64 QtMultimedia::MediaRecorder::duration

    \brief This property holds the recorded media duration in milliseconds.
*/

/*!
    \property QMediaRecorder::duration

    \brief the recorded media duration in milliseconds.
*/

qint64 QMediaRecorder::duration() const
{
    return d_func()->control ? d_func()->control->duration() : 0;
}

#if QT_DEPRECATED_SINCE(6, 9)
/*!
    \fn void QMediaRecorder::encoderSettingsChanged()
    \deprecated [6.9] Use specific signals instead.

    Signals when the encoder settings change.
*/
#endif

/*!
    \qmlmethod QtMultimedia::MediaRecorder::record()
    \brief Starts recording.

    While the recorder state is changed immediately to
    \c MediaRecorder.RecordingState, recording may start asynchronously.

    If recording fails, the error() signal is emitted with recorder state being
    reset back to \c{QMediaRecorder.StoppedState}.

    This method updates \l actualLocation according to its generation rules.

    \note On mobile devices, recording will happen in the orientation the
    device had when calling record and is locked for the duration of the recording.
    To avoid artifacts on the user interface, we recommend to keep the user interface
    locked to the same orientation as long as the recording is ongoing using
    the contentOrientation property of the Window and unlock it again once the recording
    is finished.
*/
/*!
    Starts recording.

    While the recorder state is changed immediately to
    c\{QMediaRecorder::RecordingState}, recording may start asynchronously.

    If recording fails error() signal is emitted with recorder state being
    reset back to \c{QMediaRecorder::StoppedState}.

    This method updates \l actualLocation according to its generation rules.

    \note On mobile devices, recording will happen in the orientation the
    device had when calling record and is locked for the duration of the recording.
    To avoid artifacts on the user interface, we recommend to keep the user interface
    locked to the same orientation as long as the recording is ongoing using
    the contentOrientation property of QWindow and unlock it again once the recording
    is finished.
*/

void QMediaRecorder::record()
{
    Q_D(QMediaRecorder);

    if (!d->control || !d->captureSession)
        return;

    if (d->control->state() == QMediaRecorder::PausedState) {
        d->control->resume();
    } else {
        auto oldMediaFormat = d->encoderSettings.mediaFormat();

        auto platformSession = d->captureSession->platformSession();
        const bool hasVideo = platformSession && !platformSession->activeVideoSources().empty();

        d->encoderSettings.resolveFormat(hasVideo ? QMediaFormat::RequiresVideo : QMediaFormat::NoFlags);
        d->control->clearActualLocation();
        d->control->clearError();

        auto settings = d->encoderSettings;
        d->control->record(d->encoderSettings);

#if QT_DEPRECATED_SINCE(6, 9)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
        if (settings != d->encoderSettings)
            emit encoderSettingsChanged();
QT_WARNING_POP
#endif

        if (oldMediaFormat != d->encoderSettings.mediaFormat())
            emit mediaFormatChanged();

        if (settings.encodingMode() != d->encoderSettings.encodingMode())
            emit encodingModeChanged();

        if (settings.quality() != d->encoderSettings.quality())
            emit qualityChanged();

        if (settings.videoResolution() != d->encoderSettings.videoResolution())
            emit videoResolutionChanged();

        if (!qFuzzyCompare(settings.videoFrameRate(), d->encoderSettings.videoFrameRate()))
            emit videoFrameRateChanged();

        if (settings.videoBitRate() != d->encoderSettings.videoBitRate())
            emit videoBitRateChanged();

        if (settings.audioBitRate() != d->encoderSettings.audioBitRate())
            emit audioBitRateChanged();

        if (settings.audioChannelCount() != d->encoderSettings.audioChannelCount())
            emit audioChannelCountChanged();

        if (settings.audioSampleRate() != d->encoderSettings.audioSampleRate())
            emit audioSampleRateChanged();
    }
}
/*!
    \qmlmethod QtMultimedia::MediaRecorder::pause()
    \brief Pauses recording.

    The recorder state is changed to QMediaRecorder.PausedState.

    Depending on the platform, pausing recording may be not supported.
    In this case the recorder state is unchanged.
*/
/*!
    Pauses recording.

    The recorder state is changed to QMediaRecorder::PausedState.

    Depending on the platform, pausing recording may be not supported.
    In this case the recorder state is unchanged.
*/

void QMediaRecorder::pause()
{
    Q_D(QMediaRecorder);
    if (d->control && d->captureSession)
        d->control->pause();
}
/*!
    \qmlmethod QtMultimedia::MediaRecorder::stop()
    \brief Stops the recording.

    The recorder will stop the recording. Processing pending video and audio data might
    however still take some time. The recording is finished, once the state of the media
    recorder changes to QMediaRecorder::StoppedState.
*/

/*!
    The recorder will stop the recording. Processing pending video and audio data might
    however still take some time. The recording is finished, once the state of the media
    recorder changes to QMediaRecorder::StoppedState.
*/
void QMediaRecorder::stop()
{
    Q_D(QMediaRecorder);
    if (d->control && d->captureSession)
        d->control->stop();
}
/*!
    \qmlproperty enumeration QtMultimedia::MediaRecorder::recorderState
    \brief This property holds the current media recorder state.

    The state property represents the user request and is changed synchronously
    during record(), pause() or stop() calls.
    RecorderSstate may also change asynchronously when recording fails.

    \value MediaRecorder.StoppedState The recorder is not active.
    \value MediaRecorder.RecordingState The recording is requested.
    \value MediaRecorder.PausedState The recorder is pause.
*/
/*!
    \enum QMediaRecorder::RecorderState

    \value StoppedState    The recorder is not active.
    \value RecordingState  The recording is requested.
    \value PausedState     The recorder is paused.
*/
/*!
    \qmlproperty enumeration QtMultimedia::MediaRecorder::error
    \brief This property holds the current media recorder error state.

    \value MediaRecorder.NoError Not in an error state.
    \value MediaRecorder.ResourceError Not enough system resources
    \value MediaRecorder.FormatError the current format is not supported.
    \value MediaRecorder.OutOfSpaceError No space left on device.
    \value MediaRecorder.LocationNotWriteable The output location is not writable.
*/
/*!
    \enum QMediaRecorder::Error

    \value NoError         No Errors.
    \value ResourceError   Device is not ready or not available.
    \value FormatError     Current format is not supported.
    \value OutOfSpaceError No space left on device.
    \value LocationNotWritable The output location is not writable.
*/

/*!
    \property QMediaRecorder::recorderState
    \brief The current state of the media recorder.

    The state property represents the user request and is changed synchronously
    during record(), pause() or stop() calls.
    Recorder state may also change asynchronously when recording fails.
*/

/*!
    \qmlsignal QtMultimedia::MediaRecorder::recorderStateChanged(RecorderState state)
    \brief Signals that a media recorder's \a state has changed.
*/

/*!
    \fn QMediaRecorder::recorderStateChanged(QMediaRecorder::RecorderState state)

    Signals that a media recorder's \a state has changed.
*/

/*!
    \qmlsignal QtMultimedia::MediaRecorder::durationChanged(qint64 duration)
    \brief Signals that the \a duration of the recorded media has changed.
*/

/*!
    \fn QMediaRecorder::durationChanged(qint64 duration)

    Signals that the \a duration of the recorded media has changed.
*/
/*!
    \qmlsignal QtMultimedia::MediaRecorder::actualLocationChanged(const QUrl &location)
    \brief Signals that the actual \a location of the recorded media has changed.

    This signal is usually emitted when recording starts.
*/
/*!
    \fn QMediaRecorder::actualLocationChanged(const QUrl &location)

    Signals that the actual \a location of the recorded media has changed.
    This signal is usually emitted when recording starts.
*/
/*!
    \qmlsignal QtMultimedia::MediaRecorder::errorOccurred(Error error, const QString &errorString)
    \brief Signals that an \a error has occurred.

    The \a errorString contains a description of the error.
*/
/*!
    \fn QMediaRecorder::errorOccurred(QMediaRecorder::Error error, const QString &errorString)

    Signals that an \a error has occurred, with \a errorString containing
    a description of the error.
*/

/*!
    \qmlproperty mediaMetaData QtMultimedia::MediaRecorder::metaData

    \brief This property holds meta data associated with the recording.

    When a recording is started, any meta-data assigned will be attached to  that
    recording.

    \note Ensure that meta-data is assigned correctly by assigning it before
    starting the recording.

    \sa mediaMetaData
*/

/*!
    \property QMediaRecorder::metaData

    Returns the metaData associated with the recording.
*/
QMediaMetaData QMediaRecorder::metaData() const
{
    Q_D(const QMediaRecorder);

    return d->control ? d->control->metaData() : QMediaMetaData{};
}

/*!
    Sets the meta data to \a metaData.

    \note To ensure that meta-data is set correctly, it should be set before starting the recording.
    Once the recording is started, any meta-data set will be attached to the next recording.
*/
void QMediaRecorder::setMetaData(const QMediaMetaData &metaData)
{
    Q_D(QMediaRecorder);

    if (d->control && d->captureSession)
        d->control->setMetaData(metaData);
}

/*!
    Adds \a metaData to the recorded media.
*/
void QMediaRecorder::addMetaData(const QMediaMetaData &metaData)
{
    auto data = this->metaData();
    // merge data
    for (auto &&[key, value] : metaData.asKeyValueRange())
        data.insert(key, value);
    setMetaData(data);
}

/*!
    \property QMediaRecorder::autoStop

    This property controls whether the media recorder stops automatically when
    all media inputs have reported the end of the stream or have been deactivated.

    The end of the stream is reported by sending an empty media frame,
    which you can send explicitly via \l QVideoFrameInput or \l QAudioBufferInput.

    Video inputs, specificly, \l QCamera, \l QScreenCapture and \l QWindowCapture,
    can be deactivated via the function \c setActive.

    Defaults to \c false.

    QMediaRecorder::autoStop is only supported with the FFmpeg backend.

    \sa QCamera, QScreenCapture, QWindowCapture
*/

bool QMediaRecorder::autoStop() const
{
    Q_D(const QMediaRecorder);

    return d->autoStop;
}

void QMediaRecorder::setAutoStop(bool autoStop)
{
    Q_D(QMediaRecorder);

    if (d->autoStop == autoStop)
        return;

    d->autoStop = autoStop;

    if (d->control)
        d->control->updateAutoStop();

    emit autoStopChanged();
}

/*!
    \qmlsignal QtMultimedia::MediaRecorder::metaDataChanged()

    \brief Signals that a media object's meta-data has changed.

    If multiple meta-data elements are changed metaDataChanged() is emitted
    once.
*/
/*!
    \fn QMediaRecorder::metaDataChanged()

    Signals that a media object's meta-data has changed.

    If multiple meta-data elements are changed metaDataChanged() is emitted
    once.
*/

/*!
    Returns the media capture session.
*/
QMediaCaptureSession *QMediaRecorder::captureSession() const
{
    Q_D(const QMediaRecorder);
    return d->captureSession;
}
/*!
    \qmlproperty enumeration QtMultimedia::MediaRecorder::quality

    Enumerates quality encoding levels.

    \value MediaRecorder.VeryLowQuality
    \value MediaRecorder.LowQuality
    \value MediaRecorder.NormalQuality
    \value MediaRecorder.HighQuality
    \value MediaRecorder.VeryHighQuality
*/
/*!
    \enum QMediaRecorder::Quality

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

/*!

    \qmlproperty mediaFormat QtMultimedia::MediaRecorder::mediaFormat

    \brief This property holds the current MediaFormat of the recorder.
*/
/*!
    \property QMediaRecorder::mediaFormat

    \brief This property holds the current \l QMediaFormat of the recorder.

    The value of this property may change when invoking \l record(). If this happens, the
    mediaFormatChanged() signal will be emitted. This will always happen if the
    \l QMediaFormat::audioCodec or \l QMediaFormat::fileFormat properties are set to unspecified.
    If a video source (\l QCamera, \l QScreenCapture, or \l QVideoFrameInput) is connected to the
    \l QMediaCaptureSession, \l QMediaFormat::videoCodec must also be specified.
    The \l QMediaFormat::audioCodec and \l QMediaFormat::videoCodec property values may also change
    if the media backend does not support the selected file format or codec.

    The \l QMediaFormat::fileFormat property value may also change to an \c audio only format if a
    video format was requested, but \l QMediaCaptureSession does not have a video source connected.
    For example, if \l QMediaFormat::fileFormat is set to \l QMediaFormat::MPEG4, it may
    be changed to \l QMediaFormat::Mpeg4Audio.

    Applications can determine if \c mediaFormat will change before recording starts by calling the
    \l QMediaFormat::isSupported() function. When recording without any video inputs,
    \l record() will not be changed the \l QMediaFormat if the following is true:
    \list
        \li \l QMediaFormat::fileFormat is specified
        \li \l QMediaFormat::audioCodec is specified
        \li \l QMediaFormat::videoCodec is \b{unspecified}
        \li \l QMediaFormat::isSupported() returns \c true
    \endlist
    When recording with video input, \c mediaFormat will not be changed if the following is true:
    \list
        \li \l QMediaFormat::fileFormat is specified
        \li \l QMediaFormat::audioCodec is specified
        \li \l QMediaFormat::videoCodec is specified
        \li \l QMediaFormat::isSupported() returns \c true
    \endlist

    \note The \l QMediaRecorder does not take the file name extension from the \l outputLocation
    property into account when determining the \l QMediaFormat::fileFormat, and will not adjust the
    extension of the \l outputLocation \l QUrl to match the selected file format if an extension is
    specified. Applications should therefore make sure to set the
    \l {QMediaFormat::fileFormat}{QMediaRecorder::mediaFormat::fileFormat} to match the file
    extension, or not specify a file extension. If no file extension is specified, the
    \l actualLocation file extension will be updated to match the file format used for recording.

    \sa QMediaFormat::isSupported(), actualLocation
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
    emit mediaFormatChanged();
}

/*!

    \qmlproperty enumeration QtMultimedia::MediaRecorder::encodingMode
    \since 6.6
    \brief This property holds the encoding mode.
    \sa QMediaRecorder::EncodingMode
*/

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
    \fn void QMediaRecorder::encodingModeChanged()

    Signals when the encoding mode changes.
*/
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
    emit encodingModeChanged();
}

/*!
    \property QMediaRecorder::quality

    Returns the recording quality.
*/
QMediaRecorder::Quality QMediaRecorder::quality() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.quality();
}

/*!
    \fn void QMediaRecorder::qualityChanged()

    Signals when the recording quality changes.
*/
void QMediaRecorder::setQuality(Quality quality)
{
    Q_D(QMediaRecorder);

    quality = std::clamp(quality, QMediaRecorder::Quality::VeryLowQuality,
                         QMediaRecorder::Quality::VeryHighQuality);

    if (d->encoderSettings.quality() == quality)
        return;

    d->encoderSettings.setQuality(quality);
    emit qualityChanged();
}

/*!
    \qmlproperty Size QtMultimedia::MediaRecorder::videoResolution
    \since 6.6
    \brief This property holds the resolution of the encoded video.

    Set an empty Size to make the recorder choose an optimal resolution based
    on what is available from the video source and the limitations of the codec.
*/


/*!
    Returns the resolution of the encoded video.
*/
QSize QMediaRecorder::videoResolution() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.videoResolution();
}

/*!
    \fn void QMediaRecorder::videoResolutionChanged()

    Signals when the video recording resolution changes.
*/
/*!
    Sets the resolution of the encoded video to \a{size}.

    Pass an empty QSize to make the recorder choose an optimal resolution based
    on what is available from the video source and the limitations of the codec.
*/
void QMediaRecorder::setVideoResolution(const QSize &size)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.videoResolution() == size)
        return;
    d->encoderSettings.setVideoResolution(size);
    emit videoResolutionChanged();
}

/*! \fn void QMediaRecorder::setVideoResolution(int width, int height)

    Sets the \a width and \a height of the resolution of the encoded video.

    \overload
*/

/*!
    \qmlproperty real QtMultimedia::MediaRecorder::videoFrameRate
    \since 6.6
    \brief This property holds the video frame rate.

    A value of 0 indicates the recorder should make an optimal choice based on what is available
    from the video source and the limitations of the codec.
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
    \fn void QMediaRecorder::videoFrameRateChanged()

    Signals when the recording video frame rate changes.
*/
/*!
    Sets the video \a frameRate.

    A value of 0 indicates the recorder should make an optimal choice based on what is available
    from the video source and the limitations of the codec.
*/
void QMediaRecorder::setVideoFrameRate(qreal frameRate)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.videoFrameRate() == frameRate)
        return;
    d->encoderSettings.setVideoFrameRate(frameRate);
    emit videoFrameRateChanged();
}

/*!
    \qmlproperty int QtMultimedia::MediaRecorder::videoBitRate
    \since 6.6
    \brief This property holds the bit rate of the compressed video stream in bits per second.
*/

/*!
    Returns the bit rate of the compressed video stream in bits per second.
*/
int QMediaRecorder::videoBitRate() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.videoBitRate();
}

/*!
    \fn void QMediaRecorder::videoBitRateChanged()

    Signals when the recording video bit rate changes.
*/
/*!
    Sets the video \a bitRate in bits per second.
*/
void QMediaRecorder::setVideoBitRate(int bitRate)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.videoBitRate() == bitRate)
        return;
    d->encoderSettings.setVideoBitRate(bitRate);
    emit videoBitRateChanged();
}

/*!
    \qmlproperty int QtMultimedia::MediaRecorder::audioBitRate
    \since 6.6
    \brief This property holds the bit rate of the compressed audio stream in bits per second.
*/

/*!
    Returns the bit rate of the compressed audio stream in bits per second.
*/
int QMediaRecorder::audioBitRate() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.audioBitRate();
}

/*!
    \fn void QMediaRecorder::audioBitRateChanged()

    Signals when the recording audio bit rate changes.
*/
/*!
    Sets the audio \a bitRate in bits per second.
*/
void QMediaRecorder::setAudioBitRate(int bitRate)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.audioBitRate() == bitRate)
        return;
    d->encoderSettings.setAudioBitRate(bitRate);
    emit audioBitRateChanged();
}

/*!
    \qmlproperty int QtMultimedia::MediaRecorder::audioChannelCount
    \since 6.6
    \brief This property holds the number of audio channels.
*/

/*!
    Returns the number of audio channels.
*/
int QMediaRecorder::audioChannelCount() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.audioChannelCount();
}

/*!
    \fn void QMediaRecorder::audioChannelCountChanged()

    Signals when the recording audio channel count changes.
*/
/*!
    Sets the number of audio \a channels.

    A value of -1 indicates the recorder should make an optimal choice based on
    what is available from the audio source and the limitations of the codec.
*/
void QMediaRecorder::setAudioChannelCount(int channels)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.audioChannelCount() == channels)
        return;
    d->encoderSettings.setAudioChannelCount(channels);
    emit audioChannelCountChanged();
}

/*!
    \qmlproperty int QtMultimedia::MediaRecorder::audioSampleRate
    \since 6.6
    \brief This property holds the audio sample rate in Hz.
*/

/*!
    Returns the audio sample rate in Hz.
*/
int QMediaRecorder::audioSampleRate() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.audioSampleRate();
}
/*!
    \fn void QMediaRecorder::audioSampleRateChanged()

    Signals when the recording audio sample rate changes.
*/
/*!
    Sets the audio \a sampleRate in Hz.

    A value of \c -1 indicates the recorder should make an optimal choice based
    on what is available from the audio source, and the limitations of the codec.
*/
void QMediaRecorder::setAudioSampleRate(int sampleRate)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.audioSampleRate() == sampleRate)
        return;
    d->encoderSettings.setAudioSampleRate(sampleRate);
    emit audioSampleRateChanged();
}

QT_END_NAMESPACE

#include "moc_qmediarecorder.cpp"
