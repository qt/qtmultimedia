// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidcapturesession_p.h"

#include "androidcamera_p.h"
#include "qandroidcamerasession_p.h"
#include "qaudioinput.h"
#include "qaudiooutput.h"
#include "androidmediaplayer_p.h"
#include "androidmultimediautils_p.h"
#include "qandroidmultimediautils_p.h"
#include "qandroidvideooutput_p.h"
#include "qandroidglobal_p.h"
#include <private/qplatformaudioinput_p.h>
#include <private/qplatformaudiooutput_p.h>
#include <private/qmediarecorder_p.h>
#include <private/qmediastoragelocation_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

QAndroidCaptureSession::QAndroidCaptureSession()
    : QObject()
    , m_mediaRecorder(0)
    , m_cameraSession(0)
    , m_duration(0)
    , m_state(QMediaRecorder::StoppedState)
    , m_outputFormat(AndroidMediaRecorder::DefaultOutputFormat)
    , m_audioEncoder(AndroidMediaRecorder::DefaultAudioEncoder)
    , m_videoEncoder(AndroidMediaRecorder::DefaultVideoEncoder)
{
    m_notifyTimer.setInterval(1000);
    connect(&m_notifyTimer, &QTimer::timeout, this, &QAndroidCaptureSession::updateDuration);
}

QAndroidCaptureSession::~QAndroidCaptureSession()
{
    stop();
    m_mediaRecorder = nullptr;
    if (m_audioInput && m_audioOutput)
        AndroidMediaPlayer::stopSoundStreaming();
}

void QAndroidCaptureSession::setCameraSession(QAndroidCameraSession *cameraSession)
{
    if (m_cameraSession) {
        disconnect(m_connOpenCamera);
        disconnect(m_connActiveChangedCamera);
    }

    m_cameraSession = cameraSession;
    if (m_cameraSession) {
        m_connOpenCamera = connect(cameraSession, &QAndroidCameraSession::opened,
                                   this, &QAndroidCaptureSession::onCameraOpened);
        m_connActiveChangedCamera = connect(cameraSession, &QAndroidCameraSession::activeChanged,
                                            this, [this](bool isActive) {
            if (!isActive)
                stop();
        });
    }
}

void QAndroidCaptureSession::setAudioInput(QPlatformAudioInput *input)
{
    if (m_audioInput == input)
        return;

    if (m_audioInput) {
        disconnect(m_audioInputChanged);
    }

    m_audioInput = input;

    if (m_audioInput) {
        m_audioInputChanged = connect(m_audioInput->q, &QAudioInput::deviceChanged, this, [this]() {
            if (m_state == QMediaRecorder::RecordingState)
                m_mediaRecorder->setAudioInput(m_audioInput->device.id());
            updateStreamingState();
        });
    }
    updateStreamingState();
}

void QAndroidCaptureSession::setAudioOutput(QPlatformAudioOutput *output)
{
    if (m_audioOutput == output)
        return;

    if (m_audioOutput)
        disconnect(m_audioOutputChanged);

    m_audioOutput = output;

    if (m_audioOutput) {
        m_audioOutputChanged = connect(m_audioOutput->q, &QAudioOutput::deviceChanged, this,
            [this] () {
                AndroidMediaPlayer::setAudioOutput(m_audioOutput->device.id());
                updateStreamingState();
            });
        AndroidMediaPlayer::setAudioOutput(m_audioOutput->device.id());
    }
    updateStreamingState();
}

void QAndroidCaptureSession::updateStreamingState()
{
    if (m_audioInput && m_audioOutput) {
        AndroidMediaPlayer::startSoundStreaming(m_audioInput->device.id().toInt(),
                                                m_audioOutput->device.id().toInt());
    } else {
        AndroidMediaPlayer::stopSoundStreaming();
    }
}

QMediaRecorder::RecorderState QAndroidCaptureSession::state() const
{
    return m_state;
}

void QAndroidCaptureSession::setKeepAlive(bool keepAlive)
{
    if (m_cameraSession)
        m_cameraSession->setKeepAlive(keepAlive);
}


void QAndroidCaptureSession::start(QMediaEncoderSettings &settings, const QUrl &outputLocation)
{
    if (m_state == QMediaRecorder::RecordingState)
        return;

    if (!m_cameraSession && !m_audioInput) {
        updateError(QMediaRecorder::ResourceError, QLatin1String("No devices are set"));
        return;
    }

    setKeepAlive(true);

    const bool validCameraSession = m_cameraSession && m_cameraSession->camera();

    if (validCameraSession && !qt_androidCheckCameraPermission()) {
        updateError(QMediaRecorder::ResourceError, QLatin1String("Camera permission denied."));
        setKeepAlive(false);
        return;
    }

    if (m_audioInput && !qt_androidCheckMicrophonePermission()) {
        updateError(QMediaRecorder::ResourceError, QLatin1String("Microphone permission denied."));
        setKeepAlive(false);
        return;
    }

    m_mediaRecorder = std::make_shared<AndroidMediaRecorder>();
    connect(m_mediaRecorder.get(), &AndroidMediaRecorder::error, this,
            &QAndroidCaptureSession::onError);
    connect(m_mediaRecorder.get(), &AndroidMediaRecorder::info, this,
            &QAndroidCaptureSession::onInfo);

    applySettings(settings);

    // Set audio/video sources
    if (validCameraSession) {
        m_cameraSession->camera()->stopPreviewSynchronous();
        m_cameraSession->camera()->unlock();

        m_mediaRecorder->setCamera(m_cameraSession->camera());
        m_mediaRecorder->setVideoSource(AndroidMediaRecorder::Camera);
    }

    if (m_audioInput) {
        m_mediaRecorder->setAudioInput(m_audioInput->device.id());
        if (!m_mediaRecorder->isAudioSourceSet())
            m_mediaRecorder->setAudioSource(AndroidMediaRecorder::DefaultAudioSource);
    }

    // Set output format
    m_mediaRecorder->setOutputFormat(m_outputFormat);

    // Set video encoder settings
    if (validCameraSession) {
        m_mediaRecorder->setVideoSize(settings.videoResolution());
        m_mediaRecorder->setVideoFrameRate(qRound(settings.videoFrameRate()));
        m_mediaRecorder->setVideoEncodingBitRate(settings.videoBitRate());
        m_mediaRecorder->setVideoEncoder(m_videoEncoder);

        // media recorder is also compensanting the mirror on front camera
        auto rotation = m_cameraSession->currentCameraRotation();
        if (m_cameraSession->camera()->getFacing() == AndroidCamera::CameraFacingFront)
            rotation = (360 - rotation) % 360; // remove mirror compensation

        m_mediaRecorder->setOrientationHint(rotation);
    }

    // Set audio encoder settings
    if (m_audioInput) {
        m_mediaRecorder->setAudioChannels(settings.audioChannelCount());
        m_mediaRecorder->setAudioEncodingBitRate(settings.audioBitRate());
        m_mediaRecorder->setAudioSamplingRate(settings.audioSampleRate());
        m_mediaRecorder->setAudioEncoder(m_audioEncoder);
    }

    QString extension = settings.preferredSuffix();
    // Set output file
    auto location = outputLocation.toString(QUrl::PreferLocalFile);
    QString filePath = location;
    if (QUrl(filePath).scheme() != QLatin1String("content")) {
        filePath = QMediaStorageLocation::generateFileName(
                    location, m_cameraSession ? QStandardPaths::MoviesLocation
                                              : QStandardPaths::MusicLocation, extension);
    }

    m_usedOutputLocation = QUrl::fromLocalFile(filePath);
    m_outputLocationIsStandard = location.isEmpty() || QFileInfo(location).isRelative();
    m_mediaRecorder->setOutputFile(filePath);

    if (validCameraSession) {
        m_cameraSession->disableRotation();
    }

    if (!m_mediaRecorder->prepare()) {
        updateError(QMediaRecorder::FormatError,
                    QLatin1String("Unable to prepare the media recorder."));
        restartViewfinder();

        return;
    }

    if (!m_mediaRecorder->start()) {
        updateError(QMediaRecorder::FormatError, QMediaRecorderPrivate::msgFailedStartRecording());
        restartViewfinder();

        return;
    }

    m_elapsedTime.start();
    m_notifyTimer.start();
    updateDuration();

    if (validCameraSession) {
        m_cameraSession->setReadyForCapture(false);

        // Preview frame callback is cleared when setting up the camera with the media recorder.
        // We need to reset it.
        m_cameraSession->camera()->setupPreviewFrameCallback();
    }

    m_state = QMediaRecorder::RecordingState;
    emit stateChanged(m_state);
}

void QAndroidCaptureSession::stop(bool error)
{
    if (m_state == QMediaRecorder::StoppedState || m_mediaRecorder == nullptr)
        return;

    m_mediaRecorder->stop();
    m_notifyTimer.stop();
    updateDuration();
    m_elapsedTime.invalidate();

    m_mediaRecorder = nullptr;

    if (m_cameraSession && m_cameraSession->isActive()) {
        // Viewport needs to be restarted after recording
        restartViewfinder();
    }

    if (!error) {
        // if the media is saved into the standard media location, register it
        // with the Android media scanner so it appears immediately in apps
        // such as the gallery.
        if (m_outputLocationIsStandard)
            AndroidMultimediaUtils::registerMediaFile(m_usedOutputLocation.toLocalFile());

        emit actualLocationChanged(m_usedOutputLocation);
    }

    m_state = QMediaRecorder::StoppedState;
    emit stateChanged(m_state);
}

qint64 QAndroidCaptureSession::duration() const
{
    return m_duration;
}

void QAndroidCaptureSession::applySettings(QMediaEncoderSettings &settings)
{
    // container settings
    auto fileFormat = settings.mediaFormat().fileFormat();
    if (!m_cameraSession && fileFormat == QMediaFormat::AAC) {
        m_outputFormat = AndroidMediaRecorder::AAC_ADTS;
    } else if (fileFormat == QMediaFormat::Ogg) {
        m_outputFormat = AndroidMediaRecorder::OGG;
    } else if (fileFormat == QMediaFormat::WebM) {
        m_outputFormat = AndroidMediaRecorder::WEBM;
//    } else if (fileFormat == QLatin1String("3gp")) {
//        m_outputFormat = AndroidMediaRecorder::THREE_GPP;
    } else {
        // fallback to MP4
        m_outputFormat = AndroidMediaRecorder::MPEG_4;
    }

    // audio settings
    if (settings.audioChannelCount() <= 0)
        settings.setAudioChannelCount(m_defaultSettings.audioChannels);
    if (settings.audioBitRate() <= 0)
        settings.setAudioBitRate(m_defaultSettings.audioBitRate);
    if (settings.audioSampleRate() <= 0)
        settings.setAudioSampleRate(m_defaultSettings.audioSampleRate);

    if (settings.audioCodec() == QMediaFormat::AudioCodec::AAC)
        m_audioEncoder = AndroidMediaRecorder::AAC;
    else if (settings.audioCodec() == QMediaFormat::AudioCodec::Opus)
        m_audioEncoder = AndroidMediaRecorder::OPUS;
    else if (settings.audioCodec() == QMediaFormat::AudioCodec::Vorbis)
        m_audioEncoder = AndroidMediaRecorder::VORBIS;
    else
        m_audioEncoder = m_defaultSettings.audioEncoder;


    // video settings
    if (m_cameraSession && m_cameraSession->camera()) {
        if (settings.videoResolution().isEmpty()) {
            settings.setVideoResolution(m_defaultSettings.videoResolution);
        } else if (!m_supportedResolutions.contains(settings.videoResolution())) {
            // if the requested resolution is not supported, find the closest one
            QSize reqSize = settings.videoResolution();
            int reqPixelCount = reqSize.width() * reqSize.height();
            QList<int> supportedPixelCounts;
            for (int i = 0; i < m_supportedResolutions.size(); ++i) {
                const QSize &s = m_supportedResolutions.at(i);
                supportedPixelCounts.append(s.width() * s.height());
            }
            int closestIndex = qt_findClosestValue(supportedPixelCounts, reqPixelCount);
            settings.setVideoResolution(m_supportedResolutions.at(closestIndex));
        }

        if (settings.videoFrameRate() <= 0)
            settings.setVideoFrameRate(m_defaultSettings.videoFrameRate);
        if (settings.videoBitRate() <= 0)
            settings.setVideoBitRate(m_defaultSettings.videoBitRate);

        if (settings.videoCodec() == QMediaFormat::VideoCodec::H264)
            m_videoEncoder = AndroidMediaRecorder::H264;
        else if (settings.videoCodec() == QMediaFormat::VideoCodec::H265)
            m_videoEncoder = AndroidMediaRecorder::HEVC;
        else if (settings.videoCodec() == QMediaFormat::VideoCodec::MPEG4)
            m_videoEncoder = AndroidMediaRecorder::MPEG_4_SP;
        else
            m_videoEncoder = m_defaultSettings.videoEncoder;

    }
}

void QAndroidCaptureSession::restartViewfinder()
{

    setKeepAlive(false);

    if (!m_cameraSession)
        return;

    if (m_cameraSession && m_cameraSession->camera()) {
        m_cameraSession->camera()->reconnect();
        m_cameraSession->camera()->stopPreviewSynchronous();
        m_cameraSession->camera()->startPreview();
        m_cameraSession->setReadyForCapture(true);
        m_cameraSession->enableRotation();
    }

    m_mediaRecorder = nullptr;
}

void QAndroidCaptureSession::updateDuration()
{
    if (m_elapsedTime.isValid())
        m_duration = m_elapsedTime.elapsed();

    emit durationChanged(m_duration);
}

void QAndroidCaptureSession::onCameraOpened()
{
    m_supportedResolutions.clear();
    m_supportedFramerates.clear();

    // get supported resolutions from predefined profiles
    for (int i = 0; i < 8; ++i) {
        CaptureProfile profile = getProfile(i);
        if (!profile.isNull) {
            if (i == AndroidCamcorderProfile::QUALITY_HIGH)
                m_defaultSettings = profile;

            if (!m_supportedResolutions.contains(profile.videoResolution))
                m_supportedResolutions.append(profile.videoResolution);
            if (!m_supportedFramerates.contains(profile.videoFrameRate))
                m_supportedFramerates.append(profile.videoFrameRate);
        }
    }

    std::sort(m_supportedResolutions.begin(), m_supportedResolutions.end(), qt_sizeLessThan);
    std::sort(m_supportedFramerates.begin(), m_supportedFramerates.end());

    QMediaEncoderSettings defaultSettings;
    applySettings(defaultSettings);
    m_cameraSession->applyResolution(defaultSettings.videoResolution());
}

QAndroidCaptureSession::CaptureProfile QAndroidCaptureSession::getProfile(int id)
{
    CaptureProfile profile;
    const bool hasProfile = AndroidCamcorderProfile::hasProfile(m_cameraSession->camera()->cameraId(),
                                                                AndroidCamcorderProfile::Quality(id));

    if (hasProfile) {
        AndroidCamcorderProfile camProfile = AndroidCamcorderProfile::get(m_cameraSession->camera()->cameraId(),
                                                                          AndroidCamcorderProfile::Quality(id));

        profile.outputFormat = AndroidMediaRecorder::OutputFormat(camProfile.getValue(AndroidCamcorderProfile::fileFormat));
        profile.audioEncoder = AndroidMediaRecorder::AudioEncoder(camProfile.getValue(AndroidCamcorderProfile::audioCodec));
        profile.audioBitRate = camProfile.getValue(AndroidCamcorderProfile::audioBitRate);
        profile.audioChannels = camProfile.getValue(AndroidCamcorderProfile::audioChannels);
        profile.audioSampleRate = camProfile.getValue(AndroidCamcorderProfile::audioSampleRate);
        profile.videoEncoder = AndroidMediaRecorder::VideoEncoder(camProfile.getValue(AndroidCamcorderProfile::videoCodec));
        profile.videoBitRate = camProfile.getValue(AndroidCamcorderProfile::videoBitRate);
        profile.videoFrameRate = camProfile.getValue(AndroidCamcorderProfile::videoFrameRate);
        profile.videoResolution = QSize(camProfile.getValue(AndroidCamcorderProfile::videoFrameWidth),
                                        camProfile.getValue(AndroidCamcorderProfile::videoFrameHeight));

        if (profile.outputFormat == AndroidMediaRecorder::MPEG_4)
            profile.outputFileExtension = QStringLiteral("mp4");
        else if (profile.outputFormat == AndroidMediaRecorder::THREE_GPP)
            profile.outputFileExtension = QStringLiteral("3gp");
        else if (profile.outputFormat == AndroidMediaRecorder::AMR_NB_Format)
            profile.outputFileExtension = QStringLiteral("amr");
        else if (profile.outputFormat == AndroidMediaRecorder::AMR_WB_Format)
            profile.outputFileExtension = QStringLiteral("awb");

        profile.isNull = false;
    }

    return profile;
}

void QAndroidCaptureSession::onError(int what, int extra)
{
    Q_UNUSED(what);
    Q_UNUSED(extra);
    stop(true);
    updateError(QMediaRecorder::ResourceError, QLatin1String("Unknown error."));
}

void QAndroidCaptureSession::onInfo(int what, int extra)
{
    Q_UNUSED(extra);
    if (what == 800) {
        // MEDIA_RECORDER_INFO_MAX_DURATION_REACHED
        stop();
        updateError(QMediaRecorder::OutOfSpaceError, QLatin1String("Maximum duration reached."));
    } else if (what == 801) {
        // MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED
        stop();
        updateError(QMediaRecorder::OutOfSpaceError, QLatin1String("Maximum file size reached."));
    }
}

QT_END_NAMESPACE

#include "moc_qandroidcapturesession_p.cpp"
