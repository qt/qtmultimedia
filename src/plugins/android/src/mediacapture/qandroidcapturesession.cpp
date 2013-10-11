/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qandroidcapturesession.h"

#include "jcamera.h"
#include "qandroidcamerasession.h"
#include "jmultimediautils.h"
#include "qandroidmultimediautils.h"
#include <QtCore/private/qjni_p.h>

QT_BEGIN_NAMESPACE

QAndroidCaptureSession::QAndroidCaptureSession(QAndroidCameraSession *cameraSession)
    : QObject()
    , m_mediaRecorder(0)
    , m_cameraSession(cameraSession)
    , m_audioSource(JMediaRecorder::DefaultAudioSource)
    , m_duration(0)
    , m_state(QMediaRecorder::StoppedState)
    , m_status(QMediaRecorder::UnloadedStatus)
    , m_resolutionDirty(false)
    , m_containerFormatDirty(true)
    , m_videoSettingsDirty(true)
    , m_audioSettingsDirty(true)
    , m_outputFormat(JMediaRecorder::DefaultOutputFormat)
    , m_audioEncoder(JMediaRecorder::DefaultAudioEncoder)
    , m_videoEncoder(JMediaRecorder::DefaultVideoEncoder)
{
    if (cameraSession) {
        connect(cameraSession, SIGNAL(opened()), this, SLOT(onCameraOpened()));
        connect(cameraSession, SIGNAL(statusChanged(QCamera::Status)),
                this, SLOT(onCameraStatusChanged(QCamera::Status)));
        connect(cameraSession, SIGNAL(captureModeChanged(QCamera::CaptureModes)),
                this, SLOT(onCameraCaptureModeChanged(QCamera::CaptureModes)));
    }

    m_notifyTimer.setInterval(1000);
    connect(&m_notifyTimer, SIGNAL(timeout()), this, SLOT(updateDuration()));
}

QAndroidCaptureSession::~QAndroidCaptureSession()
{
    stop();
    delete m_mediaRecorder;
}

void QAndroidCaptureSession::setAudioInput(const QString &input)
{
    if (m_audioInput == input)
        return;

    m_audioInput = input;

    if (m_audioInput == QLatin1String("default"))
        m_audioSource = JMediaRecorder::DefaultAudioSource;
    else if (m_audioInput == QLatin1String("mic"))
        m_audioSource = JMediaRecorder::Mic;
    else if (m_audioInput == QLatin1String("voice_uplink"))
        m_audioSource = JMediaRecorder::VoiceUplink;
    else if (m_audioInput == QLatin1String("voice_downlink"))
        m_audioSource = JMediaRecorder::VoiceDownlink;
    else if (m_audioInput == QLatin1String("voice_call"))
        m_audioSource = JMediaRecorder::VoiceCall;
    else if (m_audioInput == QLatin1String("voice_recognition"))
        m_audioSource = JMediaRecorder::VoiceRecognition;
    else
        m_audioSource = JMediaRecorder::DefaultAudioSource;

    emit audioInputChanged(m_audioInput);
}

QUrl QAndroidCaptureSession::outputLocation() const
{
    return m_actualOutputLocation;
}

bool QAndroidCaptureSession::setOutputLocation(const QUrl &location)
{
    if (m_requestedOutputLocation == location)
        return false;

    m_actualOutputLocation = QUrl();
    m_requestedOutputLocation = location;

    if (m_requestedOutputLocation.isEmpty())
        return true;

    if (m_requestedOutputLocation.isValid()
            && (m_requestedOutputLocation.isLocalFile() || m_requestedOutputLocation.isRelative())) {
        emit actualLocationChanged(m_requestedOutputLocation);
        return true;
    }

    m_requestedOutputLocation = QUrl();
    return false;
}

QMediaRecorder::State QAndroidCaptureSession::state() const
{
    return m_state;
}

void QAndroidCaptureSession::setState(QMediaRecorder::State state)
{
    if (m_state == state)
        return;

    switch (state) {
    case QMediaRecorder::StoppedState:
        stop();
        break;
    case QMediaRecorder::RecordingState:
        if (!start())
            return;
        break;
    case QMediaRecorder::PausedState:
        // Not supported by Android API
        qWarning("QMediaRecorder::PausedState is not supported on Android");
        return;
    }

    m_state = state;
    emit stateChanged(m_state);
}

bool QAndroidCaptureSession::start()
{
    if (m_state == QMediaRecorder::RecordingState)
        return false;

    setStatus(QMediaRecorder::LoadingStatus);

    if (m_mediaRecorder) {
        m_mediaRecorder->release();
        delete m_mediaRecorder;
    }
    m_mediaRecorder = new JMediaRecorder;
    connect(m_mediaRecorder, SIGNAL(error(int,int)), this, SLOT(onError(int,int)));
    connect(m_mediaRecorder, SIGNAL(info(int,int)), this, SLOT(onInfo(int,int)));

    // Set audio/video sources
    if (m_cameraSession) {
        if (m_cameraSession->status() != QCamera::ActiveStatus) {
            emit error(QMediaRecorder::ResourceError, QLatin1String("Camera must be active to record it."));
            setStatus(QMediaRecorder::UnloadedStatus);
            return false;
        } else {
            updateViewfinder();
            m_cameraSession->camera()->unlock();
            m_mediaRecorder->setCamera(m_cameraSession->camera());
            m_mediaRecorder->setAudioSource(JMediaRecorder::Camcorder);
            m_mediaRecorder->setVideoSource(JMediaRecorder::Camera);
        }
    } else {
        m_mediaRecorder->setAudioSource(m_audioSource);
    }

    // Set output format
    m_mediaRecorder->setOutputFormat(m_outputFormat);

    // Set audio encoder settings
    m_mediaRecorder->setAudioChannels(m_audioSettings.channelCount());
    m_mediaRecorder->setAudioEncodingBitRate(m_audioSettings.bitRate());
    m_mediaRecorder->setAudioSamplingRate(m_audioSettings.sampleRate());
    m_mediaRecorder->setAudioEncoder(m_audioEncoder);

    // Set video encoder settings
    if (m_cameraSession) {
        m_mediaRecorder->setVideoSize(m_videoSettings.resolution());
        m_mediaRecorder->setVideoFrameRate(qRound(m_videoSettings.frameRate()));
        m_mediaRecorder->setVideoEncodingBitRate(m_videoSettings.bitRate());
        m_mediaRecorder->setVideoEncoder(m_videoEncoder);

        m_mediaRecorder->setOrientationHint(m_cameraSession->currentCameraRotation());
    }


    // Set output file
    QString filePath = m_mediaStorageLocation.generateFileName(
                m_requestedOutputLocation.isLocalFile() ? m_requestedOutputLocation.toLocalFile()
                                                        : m_requestedOutputLocation.toString(),
                m_cameraSession ? QAndroidMediaStorageLocation::Camera
                                : QAndroidMediaStorageLocation::Audio,
                m_cameraSession ? QLatin1String("VID_")
                                : QLatin1String("REC_"),
                m_containerFormat);

    m_actualOutputLocation = QUrl::fromLocalFile(filePath);
    if (m_actualOutputLocation != m_requestedOutputLocation)
        emit actualLocationChanged(m_actualOutputLocation);

    m_mediaRecorder->setOutputFile(filePath);

    if (!m_mediaRecorder->prepare()) {
        emit error(QMediaRecorder::FormatError, QLatin1String("Unable to prepare the media recorder."));
        setStatus(QMediaRecorder::UnloadedStatus);
        return false;
    }

    setStatus(QMediaRecorder::LoadedStatus);
    setStatus(QMediaRecorder::StartingStatus);

    if (!m_mediaRecorder->start()) {
        emit error(QMediaRecorder::FormatError, QLatin1String("Unable to start the media recorder."));
        setStatus(QMediaRecorder::UnloadedStatus);
        return false;
    }

    setStatus(QMediaRecorder::RecordingStatus);

    m_elapsedTime.start();
    m_notifyTimer.start();
    updateDuration();

    if (m_cameraSession)
        m_cameraSession->setReadyForCapture(false);

    return true;
}

void QAndroidCaptureSession::stop(bool error)
{
    if (m_state == QMediaRecorder::StoppedState)
        return;

    setStatus(QMediaRecorder::FinalizingStatus);

    m_mediaRecorder->stop();

    m_notifyTimer.stop();
    updateDuration();
    m_elapsedTime.invalidate();

    if (m_cameraSession) {
        m_cameraSession->camera()->reconnect();
        // Viewport needs to be restarted
        m_cameraSession->camera()->startPreview();
        m_cameraSession->setReadyForCapture(true);
    }

    m_mediaRecorder->release();
    delete m_mediaRecorder;
    m_mediaRecorder = 0;

    if (!error) {
        // if the media is saved into the standard media location, register it
        // with the Android media scanner so it appears immediately in apps
        // such as the gallery.
        QString mediaPath = m_actualOutputLocation.toLocalFile();
        QString standardLoc = m_cameraSession ? JMultimediaUtils::getDefaultMediaDirectory(JMultimediaUtils::DCIM)
                                              : JMultimediaUtils::getDefaultMediaDirectory(JMultimediaUtils::Sounds);
        if (mediaPath.startsWith(standardLoc))
            JMultimediaUtils::registerMediaFile(mediaPath);
    }

    setStatus(QMediaRecorder::UnloadedStatus);
}

void QAndroidCaptureSession::setStatus(QMediaRecorder::Status status)
{
    if (m_status == status)
        return;

    m_status = status;
    emit statusChanged(m_status);
}

QMediaRecorder::Status QAndroidCaptureSession::status() const
{
    return m_status;
}

qint64 QAndroidCaptureSession::duration() const
{
    return m_duration;
}

void QAndroidCaptureSession::setContainerFormat(const QString &format)
{
    if (m_containerFormat == format)
        return;

    m_containerFormat = format;
    m_containerFormatDirty = true;
}

void QAndroidCaptureSession::setAudioSettings(const QAudioEncoderSettings &settings)
{
    if (m_audioSettings == settings)
        return;

    m_audioSettings = settings;
    m_audioSettingsDirty = true;
}

void QAndroidCaptureSession::setVideoSettings(const QVideoEncoderSettings &settings)
{
    if (!m_cameraSession || m_videoSettings == settings)
        return;

    if (m_videoSettings.resolution() != settings.resolution())
        m_resolutionDirty = true;

    m_videoSettings = settings;
    m_videoSettingsDirty = true;
}

void QAndroidCaptureSession::applySettings()
{
    // container settings
    if (m_containerFormatDirty) {
        if (m_containerFormat.isEmpty()) {
            m_containerFormat = m_defaultSettings.outputFileExtension;
            m_outputFormat = m_defaultSettings.outputFormat;
        } else if (m_containerFormat == QLatin1String("3gp")) {
            m_outputFormat = JMediaRecorder::THREE_GPP;
        } else if (!m_cameraSession && m_containerFormat == QLatin1String("amr")) {
            m_outputFormat = JMediaRecorder::AMR_NB_Format;
        } else if (!m_cameraSession && m_containerFormat == QLatin1String("awb")) {
            m_outputFormat = JMediaRecorder::AMR_WB_Format;
        } else {
            m_containerFormat = QStringLiteral("mp4");
            m_outputFormat = JMediaRecorder::MPEG_4;
        }

        m_containerFormatDirty = false;
    }

    // audio settings
    if (m_audioSettingsDirty) {
        if (m_audioSettings.channelCount() <= 0)
            m_audioSettings.setChannelCount(m_defaultSettings.audioChannels);
        if (m_audioSettings.bitRate() <= 0)
            m_audioSettings.setBitRate(m_defaultSettings.audioBitRate);
        if (m_audioSettings.sampleRate() <= 0)
            m_audioSettings.setSampleRate(m_defaultSettings.audioSampleRate);

        if (m_audioSettings.codec().isEmpty())
            m_audioEncoder = m_defaultSettings.audioEncoder;
        else if (m_audioSettings.codec() == QLatin1String("aac"))
            m_audioEncoder = JMediaRecorder::AAC;
        else if (m_audioSettings.codec() == QLatin1String("amr-nb"))
            m_audioEncoder = JMediaRecorder::AMR_NB_Encoder;
        else if (m_audioSettings.codec() == QLatin1String("amr-wb"))
            m_audioEncoder = JMediaRecorder::AMR_WB_Encoder;
        else
            m_audioEncoder = m_defaultSettings.audioEncoder;

        m_audioSettingsDirty = false;
    }

    // video settings
    if (m_cameraSession && m_videoSettingsDirty) {
        if (m_videoSettings.resolution().isEmpty()) {
            m_videoSettings.setResolution(m_defaultSettings.videoResolution);
            m_resolutionDirty = true;
        } else if (!m_supportedResolutions.contains(m_videoSettings.resolution())) {
            // if the requested resolution is not supported, find the closest one
            QSize reqSize = m_videoSettings.resolution();
            int reqPixelCount = reqSize.width() * reqSize.height();
            QList<int> supportedPixelCounts;
            for (int i = 0; i < m_supportedResolutions.size(); ++i) {
                const QSize &s = m_supportedResolutions.at(i);
                supportedPixelCounts.append(s.width() * s.height());
            }
            int closestIndex = qt_findClosestValue(supportedPixelCounts, reqPixelCount);
            m_videoSettings.setResolution(m_supportedResolutions.at(closestIndex));
            m_resolutionDirty = true;
        }

        if (m_videoSettings.frameRate() <= 0)
            m_videoSettings.setFrameRate(m_defaultSettings.videoFrameRate);
        if (m_videoSettings.bitRate() <= 0)
            m_videoSettings.setBitRate(m_defaultSettings.videoBitRate);

        if (m_videoSettings.codec().isEmpty())
            m_videoEncoder = m_defaultSettings.videoEncoder;
        else if (m_videoSettings.codec() == QLatin1String("h263"))
            m_videoEncoder = JMediaRecorder::H263;
        else if (m_videoSettings.codec() == QLatin1String("h264"))
            m_videoEncoder = JMediaRecorder::H264;
        else if (m_videoSettings.codec() == QLatin1String("mpeg4_sp"))
            m_videoEncoder = JMediaRecorder::MPEG_4_SP;
        else
            m_videoEncoder = m_defaultSettings.videoEncoder;

        m_videoSettingsDirty = false;
    }
}

void QAndroidCaptureSession::updateViewfinder()
{
    if (!m_resolutionDirty)
        return;

    m_cameraSession->camera()->stopPreview();
    m_cameraSession->adjustViewfinderSize(m_videoSettings.resolution(), false);
    m_resolutionDirty = false;
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
            if (i == 1) // QUALITY_HIGH
                m_defaultSettings = profile;

            if (!m_supportedResolutions.contains(profile.videoResolution))
                m_supportedResolutions.append(profile.videoResolution);
            if (!m_supportedFramerates.contains(profile.videoFrameRate))
                m_supportedFramerates.append(profile.videoFrameRate);
        }
    }

    qSort(m_supportedResolutions.begin(), m_supportedResolutions.end(), qt_sizeLessThan);
    qSort(m_supportedFramerates.begin(), m_supportedFramerates.end());
}

QAndroidCaptureSession::CaptureProfile QAndroidCaptureSession::getProfile(int id)
{
    CaptureProfile profile;
    bool hasProfile = QJNIObjectPrivate::callStaticMethod<jboolean>("android/media/CamcorderProfile",
                                                             "hasProfile",
                                                             "(II)Z",
                                                             m_cameraSession->camera()->cameraId(),
                                                             id);

    if (hasProfile) {
        QJNIObjectPrivate obj = QJNIObjectPrivate::callStaticObjectMethod("android/media/CamcorderProfile",
                                                                          "get",
                                                                          "(II)Landroid/media/CamcorderProfile;",
                                                                          m_cameraSession->camera()->cameraId(),
                                                                          id);


        profile.outputFormat = JMediaRecorder::OutputFormat(obj.getField<jint>("fileFormat"));
        profile.audioEncoder = JMediaRecorder::AudioEncoder(obj.getField<jint>("audioCodec"));
        profile.audioBitRate = obj.getField<jint>("audioBitRate");
        profile.audioChannels = obj.getField<jint>("audioChannels");
        profile.audioSampleRate = obj.getField<jint>("audioSampleRate");
        profile.videoEncoder = JMediaRecorder::VideoEncoder(obj.getField<jint>("videoCodec"));
        profile.videoBitRate = obj.getField<jint>("videoBitRate");
        profile.videoFrameRate = obj.getField<jint>("videoFrameRate");
        profile.videoResolution = QSize(obj.getField<jint>("videoFrameWidth"),
                                        obj.getField<jint>("videoFrameHeight"));

        if (profile.outputFormat == JMediaRecorder::MPEG_4)
            profile.outputFileExtension = QStringLiteral("mp4");
        else if (profile.outputFormat == JMediaRecorder::THREE_GPP)
            profile.outputFileExtension = QStringLiteral("3gp");
        else if (profile.outputFormat == JMediaRecorder::AMR_NB_Format)
            profile.outputFileExtension = QStringLiteral("amr");
        else if (profile.outputFormat == JMediaRecorder::AMR_WB_Format)
            profile.outputFileExtension = QStringLiteral("awb");

        profile.isNull = false;
    }

    return profile;
}

void QAndroidCaptureSession::onCameraStatusChanged(QCamera::Status status)
{
    if (status == QCamera::StoppingStatus)
        setState(QMediaRecorder::StoppedState);
}

void QAndroidCaptureSession::onCameraCaptureModeChanged(QCamera::CaptureModes mode)
{
    if (!mode.testFlag(QCamera::CaptureVideo))
        setState(QMediaRecorder::StoppedState);
}

void QAndroidCaptureSession::onError(int what, int extra)
{
    Q_UNUSED(what)
    Q_UNUSED(extra)
    stop(true);
    m_state = QMediaRecorder::StoppedState;
    emit stateChanged(m_state);
    emit error(QMediaRecorder::ResourceError, QLatin1String("Unknown error."));
}

void QAndroidCaptureSession::onInfo(int what, int extra)
{
    Q_UNUSED(extra)
    if (what == 800) {
        // MEDIA_RECORDER_INFO_MAX_DURATION_REACHED
        setState(QMediaRecorder::StoppedState);
        emit error(QMediaRecorder::OutOfSpaceError, QLatin1String("Maximum duration reached."));
    } else if (what == 801) {
        // MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED
        setState(QMediaRecorder::StoppedState);
        emit error(QMediaRecorder::OutOfSpaceError, QLatin1String("Maximum file size reached."));
    }
}

QT_END_NAMESPACE
