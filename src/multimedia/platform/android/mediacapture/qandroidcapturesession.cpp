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

#include "qandroidcapturesession_p.h"

#include "androidcamera_p.h"
#include "qandroidcamerasession_p.h"
#include "androidmultimediautils_p.h"
#include "qandroidmultimediautils_p.h"
#include "qandroidvideooutput_p.h"
#include "qandroidglobal_p.h"
#include <private/qplatformaudioinput_p.h>
#include <private/qmediarecorder_p.h>
#include <QtCore/qmimetype.h>

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
    m_mediaStorageLocation.addStorageLocation(
                QMediaStorageLocation::Movies,
                AndroidMultimediaUtils::getDefaultMediaDirectory(AndroidMultimediaUtils::DCIM));

    m_mediaStorageLocation.addStorageLocation(
                QMediaStorageLocation::Sounds,
                AndroidMultimediaUtils::getDefaultMediaDirectory(AndroidMultimediaUtils::Sounds));

    m_notifyTimer.setInterval(1000);
    connect(&m_notifyTimer, SIGNAL(timeout()), this, SLOT(updateDuration()));
}

QAndroidCaptureSession::~QAndroidCaptureSession()
{
    stop();
    delete m_mediaRecorder;
}

void QAndroidCaptureSession::setCameraSession(QAndroidCameraSession *cameraSession)
{
    if (m_cameraSession) {
        disconnect(m_connOpenCamera);
        disconnect(m_connActiveChangedCamera);
    }

    m_cameraSession = cameraSession;
    if (m_cameraSession) {
        m_connOpenCamera = connect(cameraSession, SIGNAL(opened()), this, SLOT(onCameraOpened()));
        m_connActiveChangedCamera = connect(cameraSession, &QAndroidCameraSession::activeChanged, this,
            [this](bool isActive) {
                if (!isActive)
                    stop();
            });
    }
}

void QAndroidCaptureSession::setAudioInput(QPlatformAudioInput *input)
{
    m_audioInput = input;
}

QMediaRecorder::RecorderState QAndroidCaptureSession::state() const
{
    return m_state;
}

void QAndroidCaptureSession::start(const QMediaEncoderSettings &, const QUrl &outputLocation)
{
    if (m_state == QMediaRecorder::RecordingState)
        return;

    if (m_mediaRecorder) {
        m_mediaRecorder->release();
        delete m_mediaRecorder;
        m_mediaRecorder = nullptr;
    }

    if (!m_cameraSession && !m_audioInput) {
        Q_EMIT error(QMediaRecorder::ResourceError, QLatin1String("Audio Input device not set"));
        return;
    }

    const bool granted = m_cameraSession
                       ? m_cameraSession->requestRecordingPermission()
                       : qt_androidRequestRecordingPermission();
    if (!granted) {
        Q_EMIT error(QMediaRecorder::ResourceError, QLatin1String("Permission denied."));
        return;
    }

    m_mediaRecorder = new AndroidMediaRecorder;
    connect(m_mediaRecorder, SIGNAL(error(int,int)), this, SLOT(onError(int,int)));
    connect(m_mediaRecorder, SIGNAL(info(int,int)), this, SLOT(onInfo(int,int)));

    // Set audio/video sources
    if (m_cameraSession) {
        updateResolution();
        m_cameraSession->camera()->unlock();
        m_mediaRecorder->setCamera(m_cameraSession->camera());
        m_mediaRecorder->setAudioSource(AndroidMediaRecorder::Camcorder);
        m_mediaRecorder->setVideoSource(AndroidMediaRecorder::Camera);
    }

    if (m_audioInput) {
        m_mediaRecorder->setAudioInput(m_audioInput->device.id());
        m_mediaRecorder->setAudioSource(AndroidMediaRecorder::DefaultAudioSource);
    }

    // Set output format
    m_mediaRecorder->setOutputFormat(m_outputFormat);

    // Set audio encoder settings
    m_mediaRecorder->setAudioChannels(m_encoderSettings.audioChannelCount());
    m_mediaRecorder->setAudioEncodingBitRate(m_encoderSettings.audioBitRate());
    m_mediaRecorder->setAudioSamplingRate(m_encoderSettings.audioSampleRate());
    m_mediaRecorder->setAudioEncoder(m_audioEncoder);

    // Set video encoder settings
    if (m_cameraSession) {
        m_mediaRecorder->setVideoSize(m_encoderSettings.videoResolution());
        m_mediaRecorder->setVideoFrameRate(qRound(m_encoderSettings.videoFrameRate()));
        m_mediaRecorder->setVideoEncodingBitRate(m_encoderSettings.videoBitRate());
        m_mediaRecorder->setVideoEncoder(m_videoEncoder);

        m_mediaRecorder->setOrientationHint(m_cameraSession->currentCameraRotation());
    }

    QString extension = m_encoderSettings.mimeType().preferredSuffix();

    // Set output file
    QString filePath = m_mediaStorageLocation.generateFileName(
                outputLocation.isLocalFile() ? outputLocation.toLocalFile()
                                             : outputLocation.toString(),
                m_cameraSession ? QMediaStorageLocation::Movies
                                : QMediaStorageLocation::Sounds,
                m_cameraSession ? QLatin1String("VID_")
                                : QLatin1String("REC_"),
                extension);

    m_usedOutputLocation = QUrl::fromLocalFile(filePath);
    m_mediaRecorder->setOutputFile(filePath);

    // Even though the Android doc explicitly says that calling MediaRecorder.setPreviewDisplay()
    // is not necessary when the Camera already has a Surface, it doesn't actually work on some
    // devices. For example on the Samsung Galaxy Tab 2, the camera server dies after prepare()
    // and start() if MediaRecorder.setPreviewDispaly() is not called.
    if (m_cameraSession) {
        // When using a SurfaceTexture, we need to pass a new one to the MediaRecorder, not the same
        // one that is set on the Camera or it will crash, hence the reset().
        m_cameraSession->videoOutput()->reset();
        if (m_cameraSession->videoOutput()->surfaceTexture())
            m_mediaRecorder->setSurfaceTexture(m_cameraSession->videoOutput()->surfaceTexture());
        else if (m_cameraSession->videoOutput()->surfaceHolder())
            m_mediaRecorder->setSurfaceHolder(m_cameraSession->videoOutput()->surfaceHolder());
    }

    if (!m_mediaRecorder->prepare()) {
        emit error(QMediaRecorder::FormatError, QLatin1String("Unable to prepare the media recorder."));
        if (m_cameraSession)
            restartViewfinder();
        return;
    }

    if (!m_mediaRecorder->start()) {
        emit error(QMediaRecorder::FormatError,
                   QMediaRecorderPrivate::msgFailedStartRecording());
        if (m_cameraSession)
            restartViewfinder();
        return;
    }

    m_elapsedTime.start();
    m_notifyTimer.start();
    updateDuration();

    if (m_cameraSession) {
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
    if (m_state == QMediaRecorder::StoppedState || m_mediaRecorder == 0)
        return;

    m_mediaRecorder->stop();
    m_notifyTimer.stop();
    updateDuration();
    m_elapsedTime.invalidate();
    m_mediaRecorder->release();
    delete m_mediaRecorder;
    m_mediaRecorder = 0;

    if (m_cameraSession && m_cameraSession->isActive()) {
        // Viewport needs to be restarted after recording
        restartViewfinder();
    }

    if (!error) {
        // if the media is saved into the standard media location, register it
        // with the Android media scanner so it appears immediately in apps
        // such as the gallery.
        QString mediaPath = m_usedOutputLocation.toLocalFile();
        QString standardLoc = m_cameraSession ? AndroidMultimediaUtils::getDefaultMediaDirectory(AndroidMultimediaUtils::DCIM)
                                              : AndroidMultimediaUtils::getDefaultMediaDirectory(AndroidMultimediaUtils::Sounds);
        if (mediaPath.startsWith(standardLoc))
            AndroidMultimediaUtils::registerMediaFile(mediaPath);

        emit actualLocationChanged(m_usedOutputLocation);
    }

    m_state = QMediaRecorder::StoppedState;
    emit stateChanged(m_state);
}

qint64 QAndroidCaptureSession::duration() const
{
    return m_duration;
}

void QAndroidCaptureSession::applySettings(const QMediaEncoderSettings &settings)
{
    const auto flag = m_cameraSession ? QMediaFormat::RequiresVideo
                                       : QMediaFormat::NoFlags;
    m_encoderSettings = settings;
    m_encoderSettings.resolveFormat(flag);

    // container settings
    auto fileFormat = m_encoderSettings.mediaFormat().fileFormat();
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
    if (m_encoderSettings.audioChannelCount() <= 0)
        m_encoderSettings.setAudioChannelCount(m_defaultSettings.audioChannels);
    if (m_encoderSettings.audioBitRate() <= 0)
        m_encoderSettings.setAudioBitRate(m_defaultSettings.audioBitRate);
    if (m_encoderSettings.audioSampleRate() <= 0)
        m_encoderSettings.setAudioSampleRate(m_defaultSettings.audioSampleRate);

    if (m_encoderSettings.audioCodec() == QMediaFormat::AudioCodec::AAC)
        m_audioEncoder = AndroidMediaRecorder::AAC;
    else if (m_encoderSettings.audioCodec() == QMediaFormat::AudioCodec::Opus)
        m_audioEncoder = AndroidMediaRecorder::OPUS;
    else if (m_encoderSettings.audioCodec() == QMediaFormat::AudioCodec::Vorbis)
        m_audioEncoder = AndroidMediaRecorder::VORBIS;
    else
        m_audioEncoder = m_defaultSettings.audioEncoder;


    // video settings
    if (m_cameraSession && m_cameraSession->camera()) {
        if (m_encoderSettings.videoResolution().isEmpty()) {
            m_encoderSettings.setVideoResolution(m_defaultSettings.videoResolution);
        } else if (!m_supportedResolutions.contains(m_encoderSettings.videoResolution())) {
            // if the requested resolution is not supported, find the closest one
            QSize reqSize = m_encoderSettings.videoResolution();
            int reqPixelCount = reqSize.width() * reqSize.height();
            QList<int> supportedPixelCounts;
            for (int i = 0; i < m_supportedResolutions.size(); ++i) {
                const QSize &s = m_supportedResolutions.at(i);
                supportedPixelCounts.append(s.width() * s.height());
            }
            int closestIndex = qt_findClosestValue(supportedPixelCounts, reqPixelCount);
            m_encoderSettings.setVideoResolution(m_supportedResolutions.at(closestIndex));
        }

        if (m_encoderSettings.videoFrameRate() <= 0)
            m_encoderSettings.setVideoFrameRate(m_defaultSettings.videoFrameRate);
        if (m_encoderSettings.videoBitRate() <= 0)
            m_encoderSettings.setVideoBitRate(m_defaultSettings.videoBitRate);

        if (m_encoderSettings.videoCodec() == QMediaFormat::VideoCodec::H264)
            m_videoEncoder = AndroidMediaRecorder::H264;
        else if (m_encoderSettings.videoCodec() == QMediaFormat::VideoCodec::H265)
            m_videoEncoder = AndroidMediaRecorder::HEVC;
        else if (m_encoderSettings.videoCodec() == QMediaFormat::VideoCodec::MPEG4)
            m_videoEncoder = AndroidMediaRecorder::MPEG_4_SP;
        else
            m_videoEncoder = m_defaultSettings.videoEncoder;

    }
}

void QAndroidCaptureSession::updateResolution()
{
    m_cameraSession->camera()->stopPreviewSynchronous();
    m_cameraSession->applyResolution(m_encoderSettings.videoResolution(), false);
}

void QAndroidCaptureSession::restartViewfinder()
{
    if (!m_cameraSession)
        return;

    m_cameraSession->camera()->reconnect();

    // This is not necessary on most devices, but it crashes on some if we don't stop the
    // preview and reset the preview display on the camera when recording is over.
    m_cameraSession->camera()->stopPreviewSynchronous();
    m_cameraSession->videoOutput()->reset();
    if (m_cameraSession->videoOutput()->surfaceTexture())
        m_cameraSession->camera()->setPreviewTexture(m_cameraSession->videoOutput()->surfaceTexture());
    else if (m_cameraSession->videoOutput()->surfaceHolder())
        m_cameraSession->camera()->setPreviewDisplay(m_cameraSession->videoOutput()->surfaceHolder());

    m_cameraSession->camera()->startPreview();
    m_cameraSession->setReadyForCapture(true);
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

    applySettings(m_encoderSettings);
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
    emit error(QMediaRecorder::ResourceError, QLatin1String("Unknown error."));
}

void QAndroidCaptureSession::onInfo(int what, int extra)
{
    Q_UNUSED(extra);
    if (what == 800) {
        // MEDIA_RECORDER_INFO_MAX_DURATION_REACHED
        stop();
        emit error(QMediaRecorder::OutOfSpaceError, QLatin1String("Maximum duration reached."));
    } else if (what == 801) {
        // MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED
        stop();
        emit error(QMediaRecorder::OutOfSpaceError, QLatin1String("Maximum file size reached."));
    }
}

QT_END_NAMESPACE
