/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qstring.h>
#include <QtCore/qdir.h>
#include <QtCore/qtimer.h>

#include "s60videocapturesession.h"
#include "s60cameraconstants.h"

#include <utf.h>
#include <bautils.h>

#ifdef S60_DEVVIDEO_RECORDING_SUPPORTED
#include <mmf/devvideo/devvideorecord.h>
#endif

S60VideoCaptureSession::S60VideoCaptureSession(QObject *parent) :
    QObject(parent),
    m_cameraEngine(0),
    m_videoRecorder(0),
    m_position(0),
    m_error(KErrNone),
    m_cameraStarted(false),
    m_captureState(ENotInitialized),    // Default state
    m_sink(QUrl()),
    m_requestedSink(QUrl()),
    m_captureSettingsSet(false),
    m_container(QString()),
    m_requestedContainer(QString()),
    m_muted(false),
    m_maxClipSize(-1),
    m_videoControllerMap(QHash<int, QHash<int,VideoFormatData> >()),
    m_videoParametersForEncoder(QList<MaxResolutionRatesAndTypes>()),
    m_openWhenReady(false),
    m_prepareAfterOpenComplete(false),
    m_startAfterPrepareComplete(false),
    m_uncommittedSettings(false),
    m_commitSettingsWhenReady(false)
{
#ifdef S60_DEVVIDEO_RECORDING_SUPPORTED
    // Populate info of supported codecs, and their resolution, etc.
    TRAPD(err, doPopulateVideoCodecsDataL());
    setError(err, tr("Failed to gather video codec information."));
#endif // S60_DEVVIDEO_RECORDING_SUPPORTED

    initializeVideoCaptureSettings();

    m_durationTimer = new QTimer;
    m_durationTimer->setInterval(KDurationChangedInterval);
    connect(m_durationTimer, SIGNAL(timeout()), this, SLOT(durationTimerTriggered()));
}

S60VideoCaptureSession::~S60VideoCaptureSession()
{
    if (m_captureState >= ERecording)
        m_videoRecorder->Stop();

    if (m_captureState >= EInitialized)
        m_videoRecorder->Close();

    if (m_videoRecorder) {
        delete m_videoRecorder;
        m_videoRecorder = 0;
    }

    if (m_durationTimer) {
        delete m_durationTimer;
        m_durationTimer = 0;
    }

    // Clear all data structures
    foreach (MaxResolutionRatesAndTypes structure, m_videoParametersForEncoder) {
        structure.frameRatePictureSizePair.clear();
        structure.mimeTypes.clear();
    }
    m_videoParametersForEncoder.clear();

    m_videoCodecList.clear();
    m_audioCodecList.clear();

    QList<TInt> controllers = m_videoControllerMap.keys();
    for (int i = 0; i < controllers.size(); ++i) {
        foreach(VideoFormatData data, m_videoControllerMap[controllers[i]]){
            data.supportedMimeTypes.clear();
        }
        m_videoControllerMap[controllers[i]].clear();
    }
    m_videoControllerMap.clear();
}

/*
 * This function can be used both internally and from Control classes using this session.
 * The error notification will go to client application through QMediaRecorder error signal.
 */
void S60VideoCaptureSession::setError(const TInt error, const QString &description)
{
    if (error == KErrNone)
        return;

    m_error = error;
    QMediaRecorder::Error recError = fromSymbianErrorToQtMultimediaError(m_error);

    // Stop/Close/Reset only of other than "not supported" error
    if (m_error != KErrNotSupported) {
        if (m_captureState >= ERecording)
            m_videoRecorder->Stop();

        if (m_captureState >= EInitialized)
            m_videoRecorder->Close();

        // Reset state
        if (m_captureState != ENotInitialized) {
            if (m_durationTimer->isActive())
                m_durationTimer->stop();
            m_captureState = ENotInitialized;
            emit stateChanged(m_captureState);
        }
    }

    emit this->error(recError, description);

    // Reset only of other than "not supported" error
    if (m_error != KErrNotSupported)
        resetSession(true);
    else
        m_error = KErrNone; // Reset error
}

QMediaRecorder::Error S60VideoCaptureSession::fromSymbianErrorToQtMultimediaError(int aError)
{
    switch(aError) {
        case KErrNone:
            return QMediaRecorder::NoError; // No errors have occurred
        case KErrArgument:
        case KErrNotSupported:
            return QMediaRecorder::FormatError; // The feature/format is not supported
        case KErrNoMemory:
        case KErrNotFound:
        case KErrBadHandle:
            return QMediaRecorder::ResourceError; // Not able to use camera/recorder resources

        default:
            return QMediaRecorder::ResourceError; // Other error has occurred
    }
}

/*
 * This function applies all recording settings to make latency during the
 * start of the recording as short as possible. After this it is not possible to
 * set settings (inc. output location) before stopping the recording.
 */
void S60VideoCaptureSession::applyAllSettings()
{
    switch (m_captureState) {
    case ENotInitialized:
    case EInitializing:
        m_commitSettingsWhenReady = true;
        return;
    case EInitialized:
        setOutputLocation(QUrl());
        m_prepareAfterOpenComplete = true;
        return;
    case EOpening:
        m_prepareAfterOpenComplete = true;
        return;
    case EOpenComplete:
        // Do nothing, ready to commit
        break;
    case EPreparing:
        m_commitSettingsWhenReady = true;
        return;
    case EPrepared:
        // Revert state internally, since logically applying settings means going
        // from OpenComplete ==> Preparing ==> Prepared.
        m_captureState = EOpenComplete;
        break;
    case ERecording:
    case EPaused:
        setError(KErrNotReady, tr("Cannot apply settings while recording."));
        return;

    default:
        setError(KErrGeneral, tr("Unexpected camera error."));
        return;
    }

    // Commit settings - State is now OpenComplete (possibly reverted from Prepared)
    commitVideoEncoderSettings();

    // If capture state has been changed to:
    // * Opening: A different media container has been requested
    // * Other: Failure during the setting committing
    // ==> Return
    if (m_captureState != EOpenComplete)
        return;

    // Start preparing
    m_captureState = EPreparing;
    emit stateChanged(m_captureState);

    if (m_cameraEngine->IsCameraReady())
        m_videoRecorder->Prepare();
}

void S60VideoCaptureSession::setCameraHandle(CCameraEngine* cameraHandle)
{
    m_cameraEngine = cameraHandle;

    // Initialize settings for the new camera
    initializeVideoCaptureSettings();

    resetSession();
}

void S60VideoCaptureSession::notifySettingsSet()
{
    m_captureSettingsSet = true;
}

void S60VideoCaptureSession::doInitializeVideoRecorderL()
{
    if (m_captureState > ENotInitialized)
        resetSession();

    m_captureState = EInitializing;
    emit stateChanged(m_captureState);

    // Open Dummy file to be able to query supported settings
    int cameraHandle = m_cameraEngine->Camera() ? m_cameraEngine->Camera()->Handle() : 0;

    TUid controllerUid;
    TUid formatUid;
    selectController(m_requestedContainer, controllerUid, formatUid);

    if (m_videoRecorder) {
        // File open completes in MvruoOpenComplete
        TRAPD(err, m_videoRecorder->OpenFileL(KDummyVideoFile, cameraHandle, controllerUid, formatUid));
        setError(err, tr("Failed to initialize video recorder."));
        m_container = m_requestedContainer;
    } else {
        setError(KErrNotReady, tr("Unexpected camera error."));
    }
}

void S60VideoCaptureSession::resetSession(bool errorHandling)
{
    if (m_videoRecorder) {
        delete m_videoRecorder;
        m_videoRecorder = 0;
    }

    if (m_captureState != ENotInitialized) {
        if (m_durationTimer->isActive())
            m_durationTimer->stop();
        m_captureState = ENotInitialized;
        emit stateChanged(m_captureState);
    }

    // Reset error to be able to recover
    m_error = KErrNone;

    // Reset flags
    m_openWhenReady = false;
    m_prepareAfterOpenComplete = false;
    m_startAfterPrepareComplete = false;
    m_uncommittedSettings = false;
    m_commitSettingsWhenReady = false;

    TRAPD(err, m_videoRecorder = CVideoRecorderUtility::NewL(*this));
    if (err) {
        qWarning("Failed to create video recorder.");
        if (errorHandling)
            emit error(QMediaRecorder::ResourceError, tr("Failed to recover from video error."));
        else
            setError(err, tr("Failure in creation of video recorder device."));
        return;
    }

    updateVideoCaptureContainers();
}

QList<QSize> S60VideoCaptureSession::supportedVideoResolutions(bool *continuous)
{
    QList<QSize> resolutions;

    // Secondary Camera
    if (m_cameraEngine->CurrentCameraIndex() != 0) {
        TCameraInfo *info = m_cameraEngine->CameraInfo();
        if (info) {
            TInt videoResolutionCount = info->iNumVideoFrameSizesSupported;
            CCamera *camera = m_cameraEngine->Camera();
            if (camera) {
                for (TInt i = 0; i < videoResolutionCount; ++i) {
                    TSize checkedResolution;
                    camera->EnumerateVideoFrameSizes(checkedResolution, i, CCamera::EFormatYUV420Planar);
                    QSize qtResolution(checkedResolution.iWidth, checkedResolution.iHeight);
                    if (!resolutions.contains(qtResolution))
                        resolutions.append(qtResolution);
                }
            } else {
                setError(KErrGeneral, tr("Could not query supported video resolutions."));
            }
        } else {
            setError(KErrGeneral, tr("Could not query supported video resolutions."));
        }

    // Primary Camera
    } else {

        if (m_videoParametersForEncoder.count() > 0) {

            // Also arbitrary resolutions are supported
            if (continuous)
                *continuous = true;

            // Append all supported resolutions to the list
            foreach (MaxResolutionRatesAndTypes parameters, m_videoParametersForEncoder)
                for (int i = 0; i < parameters.frameRatePictureSizePair.count(); ++i)
                    if (!resolutions.contains(parameters.frameRatePictureSizePair[i].frameSize))
                        resolutions.append(parameters.frameRatePictureSizePair[i].frameSize);
        }
    }

#ifdef Q_CC_NOKIAX86 // Emulator
    resolutions << QSize(160, 120);
    resolutions << QSize(352, 288);
    resolutions << QSize(640,480);
#endif // Q_CC_NOKIAX86

    return resolutions;
}

QList<QSize> S60VideoCaptureSession::supportedVideoResolutions(const QVideoEncoderSettings &settings, bool *continuous)
{
    QList<QSize> supportedFrameSizes;

    // Secondary Camera
    if (m_cameraEngine->CurrentCameraIndex() != 0) {
        TCameraInfo *info = m_cameraEngine->CameraInfo();
        if (info) {
            TInt videoResolutionCount = info->iNumVideoFrameSizesSupported;
            CCamera *camera = m_cameraEngine->Camera();
            if (camera) {
                for (TInt i = 0; i < videoResolutionCount; ++i) {
                    TSize checkedResolution;
                    camera->EnumerateVideoFrameSizes(checkedResolution, i, CCamera::EFormatYUV420Planar);
                    QSize qtResolution(checkedResolution.iWidth, checkedResolution.iHeight);
                    if (!supportedFrameSizes.contains(qtResolution))
                        supportedFrameSizes.append(qtResolution);
                }
            } else {
                setError(KErrGeneral, tr("Could not query supported video resolutions."));
            }
        } else {
            setError(KErrGeneral, tr("Could not query supported video resolutions."));
        }

    // Primary Camera
    } else {

        if (settings.codec().isEmpty())
            return supportedFrameSizes;

        if (!m_videoCodecList.contains(settings.codec(), Qt::CaseInsensitive))
            return supportedFrameSizes;

        // Also arbitrary resolutions are supported
        if (continuous)
            *continuous = true;

        // Find maximum resolution (using defined framerate if set)
        for (int i = 0; i < m_videoParametersForEncoder.count(); ++i) {
            // Check if encoder supports the requested codec
            if (!m_videoParametersForEncoder[i].mimeTypes.contains(settings.codec(), Qt::CaseInsensitive))
                continue;

            foreach (SupportedFrameRatePictureSize pair, m_videoParametersForEncoder[i].frameRatePictureSizePair) {
                if (!supportedFrameSizes.contains(pair.frameSize)) {
                    QSize maxForMime = maximumResolutionForMimeType(settings.codec());
                    if (settings.frameRate() != 0) {
                        if (settings.frameRate() <= pair.frameRate) {
                            if ((pair.frameSize.width() * pair.frameSize.height()) <= (maxForMime.width() * maxForMime.height()))
                                supportedFrameSizes.append(pair.frameSize);
                        }
                    } else {
                        if ((pair.frameSize.width() * pair.frameSize.height()) <= (maxForMime.width() * maxForMime.height()))
                            supportedFrameSizes.append(pair.frameSize);
                    }
                }
            }
        }
    }

#ifdef Q_CC_NOKIAX86 // Emulator
    supportedFrameSizes << QSize(160, 120);
    supportedFrameSizes << QSize(352, 288);
    supportedFrameSizes << QSize(640,480);
#endif

    return supportedFrameSizes;
}

QList<qreal> S60VideoCaptureSession::supportedVideoFrameRates(bool *continuous)
{
    QList<qreal> supportedRatesList;

    if (m_videoParametersForEncoder.count() > 0) {
        // Insert min and max to the list
        supportedRatesList.append(1.0); // Use 1fps as sensible minimum
        qreal foundMaxFrameRate(0.0);

        // Also arbitrary framerates are supported
        if (continuous)
            *continuous = true;

        // Find max framerate
        foreach (MaxResolutionRatesAndTypes parameters, m_videoParametersForEncoder) {
            for (int i = 0; i < parameters.frameRatePictureSizePair.count(); ++i) {
                qreal maxFrameRate = parameters.frameRatePictureSizePair[i].frameRate;
                if (maxFrameRate > foundMaxFrameRate)
                    foundMaxFrameRate = maxFrameRate;
            }
        }

        supportedRatesList.append(foundMaxFrameRate);
    }

    // Add also other standard framerates to the list
    if (!supportedRatesList.isEmpty()) {
        if (supportedRatesList.last() > 30.0) {
            if (!supportedRatesList.contains(30.0))
                supportedRatesList.insert(1, 30.0);
        }
        if (supportedRatesList.last() > 25.0) {
            if (!supportedRatesList.contains(25.0))
                supportedRatesList.insert(1, 25.0);
        }
        if (supportedRatesList.last() > 15.0) {
            if (!supportedRatesList.contains(15.0))
                supportedRatesList.insert(1, 15.0);
        }
        if (supportedRatesList.last() > 10.0) {
            if (!supportedRatesList.contains(10))
                supportedRatesList.insert(1, 10.0);
        }
    }

#ifdef Q_CC_NOKIAX86 // Emulator
    supportedRatesList << 30.0 << 25.0 << 15.0 << 10.0 << 5.0;
#endif

    return supportedRatesList;
}

QList<qreal> S60VideoCaptureSession::supportedVideoFrameRates(const QVideoEncoderSettings &settings, bool *continuous)
{
    QList<qreal> supportedFrameRates;

    if (settings.codec().isEmpty())
        return supportedFrameRates;
    if (!m_videoCodecList.contains(settings.codec(), Qt::CaseInsensitive))
        return supportedFrameRates;

    // Also arbitrary framerates are supported
    if (continuous)
        *continuous = true;

    // Find maximum framerate (using defined resolution if set)
    for (int i = 0; i < m_videoParametersForEncoder.count(); ++i) {
        // Check if encoder supports the requested codec
        if (!m_videoParametersForEncoder[i].mimeTypes.contains(settings.codec(), Qt::CaseInsensitive))
            continue;

        foreach (SupportedFrameRatePictureSize pair, m_videoParametersForEncoder[i].frameRatePictureSizePair) {
            if (!supportedFrameRates.contains(pair.frameRate)) {
                qreal maxRateForMime = maximumFrameRateForMimeType(settings.codec());
                if (settings.resolution().width() != 0 && settings.resolution().height() != 0) {
                    if((settings.resolution().width() * settings.resolution().height()) <= (pair.frameSize.width() * pair.frameSize.height())) {
                        if (pair.frameRate <= maxRateForMime)
                            supportedFrameRates.append(pair.frameRate);
                    }
                } else {
                    if (pair.frameRate <= maxRateForMime)
                        supportedFrameRates.append(pair.frameRate);
                }
            }
        }
    }

    // Add also other standard framerates to the list
    if (!supportedFrameRates.isEmpty()) {
        if (supportedFrameRates.last() > 30.0) {
            if (!supportedFrameRates.contains(30.0))
                supportedFrameRates.insert(1, 30.0);
        }
        if (supportedFrameRates.last() > 25.0) {
            if (!supportedFrameRates.contains(25.0))
                supportedFrameRates.insert(1, 25.0);
        }
        if (supportedFrameRates.last() > 15.0) {
            if (!supportedFrameRates.contains(15.0))
                supportedFrameRates.insert(1, 15.0);
        }
        if (supportedFrameRates.last() > 10.0) {
            if (!supportedFrameRates.contains(10))
                supportedFrameRates.insert(1, 10.0);
        }
    }

#ifdef Q_CC_NOKIAX86 // Emulator
    supportedFrameRates << 30.0 << 25.0 << 15.0 << 10.0 << 5.0;
#endif

    return supportedFrameRates;
}

bool S60VideoCaptureSession::setOutputLocation(const QUrl &sink)
{
    m_requestedSink = sink;

    if (m_error)
        return false;

    switch (m_captureState) {
        case ENotInitialized:
        case EInitializing:
        case EOpening:
        case EPreparing:
            m_openWhenReady = true;
            return true;

        case EInitialized:
        case EOpenComplete:
        case EPrepared:
            // Continue
            break;

        case ERecording:
        case EPaused:
            setError(KErrNotReady, tr("Cannot set file name while recording."));
            return false;

        default:
            setError(KErrGeneral, tr("Unexpected camera error."));
            return false;
    }

    // Empty URL - Use default file name and path (C:\Data\Videos\video.mp4)
    if (sink.isEmpty()) {

        // Make sure default directory exists
        QDir videoDir(QDir::rootPath());
        if (!videoDir.exists(KDefaultVideoPath))
            videoDir.mkpath(KDefaultVideoPath);
        QString defaultFile = KDefaultVideoPath;
        defaultFile.append("\\");
        defaultFile.append(KDefaultVideoFileName);
        m_sink.setUrl(defaultFile);

    } else { // Non-empty URL

        QString fullUrl = sink.scheme();

        // Relative URL
        if (sink.isRelative()) {

            // Extract file name and path from the URL
            fullUrl = KDefaultVideoPath;
            fullUrl.append("\\");
            fullUrl.append(QDir::toNativeSeparators(sink.path()));

        // Absolute URL
        } else {

            // Extract file name and path from the URL
            if (fullUrl == "file") {
                fullUrl = QDir::toNativeSeparators(sink.path().right(sink.path().length() - 1));
            } else {
                fullUrl.append(":");
                fullUrl.append(QDir::toNativeSeparators(sink.path()));
            }
        }

        QString fileName = fullUrl.right(fullUrl.length() - fullUrl.lastIndexOf("\\") - 1);
        QString directory = fullUrl.left(fullUrl.lastIndexOf("\\"));
        if (directory.lastIndexOf("\\") == (directory.length() - 1))
            directory = directory.left(directory.length() - 1);

        // URL is Absolute path, not including file name
        if (!fileName.contains(".")) {
            if (fileName != "") {
                directory.append("\\");
                directory.append(fileName);
            }
            fileName = KDefaultVideoFileName;
        }

        // Make sure absolute directory exists
        QDir videoDir(QDir::rootPath());
        if (!videoDir.exists(directory))
            videoDir.mkpath(directory);

        QString resolvedURL = directory;
        resolvedURL.append("\\");
        resolvedURL.append(fileName);
        m_sink = QUrl(resolvedURL);
    }

    // State is either Initialized, OpenComplete or Prepared, Close previously opened file
    if (m_videoRecorder)
        m_videoRecorder->Close();
    else
        setError(KErrNotReady, tr("Unexpected camera error."));

    // Open file

    QString fileName = QDir::toNativeSeparators(m_sink.toString());
    TPtrC16 fileSink(reinterpret_cast<const TUint16*>(fileName.utf16()));

    int cameraHandle = m_cameraEngine->Camera() ? m_cameraEngine->Camera()->Handle() : 0;

    TUid controllerUid;
    TUid formatUid;
    selectController(m_requestedContainer, controllerUid, formatUid);

    if (m_videoRecorder) {
        // File open completes in MvruoOpenComplete
        TRAPD(err, m_videoRecorder->OpenFileL(fileSink, cameraHandle, controllerUid, formatUid));
        setError(err, tr("Failed to initialize video recorder."));
        m_container = m_requestedContainer;
        m_captureState = EOpening;
        emit stateChanged(m_captureState);
    }
    else
        setError(KErrNotReady, tr("Unexpected camera error."));

    m_uncommittedSettings = true;
    return true;
}

QUrl S60VideoCaptureSession::outputLocation() const
{
    return m_sink;
}

qint64 S60VideoCaptureSession::position()
{
    // Update position only if recording is ongoing
    if ((m_captureState == ERecording) && m_videoRecorder) {
        // Signal will be automatically emitted of position changes
        TRAPD(err, m_position = m_videoRecorder->DurationL().Int64() / 1000);
        setError(err, tr("Cannot retrieve video position."));
    }

    return m_position;
}

S60VideoCaptureSession::TVideoCaptureState S60VideoCaptureSession::state() const
{
    return m_captureState;
}

bool S60VideoCaptureSession::isMuted() const
{
    return m_muted;
}

void S60VideoCaptureSession::setMuted(const bool muted)
{
    // CVideoRecorderUtility can mute/unmute only if not recording
    if (m_captureState > EPrepared) {
        if (muted)
            setError(KErrNotSupported, tr("Muting audio is not supported during recording."));
        else
            setError(KErrNotSupported, tr("Unmuting audio is not supported during recording."));
        return;
    }

    // Check if request is already active
    if (muted == isMuted())
        return;

    m_muted = muted;

    m_uncommittedSettings = true;
}

void S60VideoCaptureSession::commitVideoEncoderSettings()
{
    if (m_captureState == EOpenComplete) {

        if (m_container != m_requestedContainer) {
            setOutputLocation(m_requestedSink);
            return;
        }

        TRAPD(err, doSetCodecsL());
        if (err) {
            setError(err, tr("Failed to set audio or video codec."));
            m_audioSettings.setCodec(KMimeTypeDefaultAudioCodec);
            m_videoSettings.setCodec(KMimeTypeDefaultVideoCodec);
        }

        doSetVideoResolution(m_videoSettings.resolution());
        doSetFrameRate(m_videoSettings.frameRate());
        doSetBitrate(m_videoSettings.bitRate());

        // Audio/Video EncodingMode are not supported in Symbian

#ifndef S60_31_PLATFORM
        if (m_audioSettings.sampleRate() != -1 && m_audioSettings.sampleRate() != 0) {
            TRAP(err, m_videoRecorder->SetAudioSampleRateL((TInt)m_audioSettings.sampleRate()));
            if (err != KErrNotSupported) {
                setError(err, tr("Setting audio sample rate failed."));
            } else {
                setError(err, tr("Setting audio sample rate is not supported."));
                m_audioSettings.setSampleRate(KDefaultSampleRate); // Reset
            }
        }
#endif // S60_31_PLATFORM

        TRAP(err, m_videoRecorder->SetAudioBitRateL((TInt)m_audioSettings.bitRate()));
        if (err != KErrNotSupported) {
            if (err == KErrArgument) {
                setError(KErrNotSupported, tr("Requested audio bitrate is not supported or previously set codec is not supported with requested bitrate."));
                int fallback = 16000;
                TRAP(err, m_videoRecorder->SetAudioBitRateL(TInt(fallback)));
                if (err == KErrNone)
                    m_audioSettings.setBitRate(fallback);
            } else {
                setError(err, tr("Setting audio bitrate failed."));
            }
        }

#ifndef S60_31_PLATFORM
        if (m_audioSettings.channelCount() != -1) {
            TRAP(err, m_videoRecorder->SetAudioChannelsL(TUint(m_audioSettings.channelCount())));
            if (err != KErrNotSupported) {
                setError(err, tr("Setting audio channel count failed."));
            } else {
                setError(err, tr("Setting audio channel count is not supported."));
                m_audioSettings.setChannelCount(KDefaultChannelCount); // Reset
            }
        }
#endif // S60_31_PLATFORM

        TBool isAudioMuted = EFalse;
        TRAP(err, isAudioMuted = !m_videoRecorder->AudioEnabledL());
        if (err != KErrNotSupported && err != KErrNone)
            setError(err, tr("Failure when checking if audio is enabled."));

        if (m_muted != (bool)isAudioMuted) {
            TRAP(err, m_videoRecorder->SetAudioEnabledL(TBool(!m_muted)));
            if (err) {
                if (err != KErrNotSupported) {
                    setError(err, tr("Failed to mute/unmute audio."));
                } else {
                    setError(err, tr("Muting/unmuting audio is not supported."));
                }
            }
            else
                emit mutedChanged(m_muted);
        }

        m_uncommittedSettings = false; // Reset
    }
}

void S60VideoCaptureSession::queryAudioEncoderSettings()
{
    if (!m_videoRecorder)
        return;

    switch (m_captureState) {
    case ENotInitialized:
    case EInitializing:
    case EOpening:
    case EPreparing:
        return;

    // Possible to query settings from CVideoRecorderUtility
    case EInitialized:
    case EOpenComplete:
    case EPrepared:
    case ERecording:
    case EPaused:
        break;

    default:
        return;
    }

    TInt err = KErrNone;

    // Codec
    TFourCC audioCodec;
    TRAP(err, audioCodec = m_videoRecorder->AudioTypeL());
    if (err) {
        if (err != KErrNotSupported)
            setError(err, tr("Querying audio codec failed."));
    }
    QString codec = "";
    foreach (TFourCC aCodec, m_audioCodecList) {
        if (audioCodec == aCodec)
            codec = m_audioCodecList.key(aCodec);
    }
    m_audioSettings.setCodec(codec);

#ifndef S60_31_PLATFORM
    // Samplerate
    TInt sampleRate = -1;
    TRAP(err, sampleRate = m_videoRecorder->AudioSampleRateL());
    if (err) {
        if (err != KErrNotSupported)
            setError(err, tr("Querying audio sample rate failed."));
    }
    m_audioSettings.setSampleRate(int(sampleRate));
#endif // S60_31_PLATFORM

    // BitRate
    TInt bitRate = -1;
    TRAP(err, bitRate = m_videoRecorder->AudioBitRateL());
    if (err) {
        if (err != KErrNotSupported)
            setError(err, tr("Querying audio bitrate failed."));
    }
    m_audioSettings.setBitRate(int(bitRate));

#ifndef S60_31_PLATFORM
    // ChannelCount
    TUint channelCount = 0;
    TRAP(err, channelCount = m_videoRecorder->AudioChannelsL());
    if (err) {
        if (err != KErrNotSupported)
            setError(err, tr("Querying audio channel count failed."));
    }
    if (channelCount != 0)
        m_audioSettings.setChannelCount(int(channelCount));
    else
        m_audioSettings.setChannelCount(-1);
#endif // S60_31_PLATFORM

    // EncodingMode
    m_audioSettings.setEncodingMode(QtMultimediaKit::ConstantQualityEncoding);

    // IsMuted
    TBool isEnabled = ETrue;
    TRAP(err, isEnabled = m_videoRecorder->AudioEnabledL());
    if (err) {
        if (err != KErrNotSupported)
            setError(err, tr("Querying whether audio is muted failed."));
    }
    m_muted = bool(!isEnabled);
}

void S60VideoCaptureSession::queryVideoEncoderSettings()
{
    if (!m_videoRecorder)
        return;

    switch (m_captureState) {
    case ENotInitialized:
    case EInitializing:
    case EOpening:
    case EPreparing:
        return;

    // Possible to query settings from CVideoRecorderUtility
    case EInitialized:
    case EOpenComplete:
    case EPrepared:
    case ERecording:
    case EPaused:
        break;

    default:
        return;
    }

    TInt err = KErrNone;

    // Codec
    const TDesC8 &videoMimeType = m_videoRecorder->VideoFormatMimeType();
    QString videoMimeTypeString = "";
    if (videoMimeType.Length() > 0) {
        // First convert the 8-bit descriptor to Unicode
        HBufC16* videoCodec;
        videoCodec = CnvUtfConverter::ConvertToUnicodeFromUtf8L(videoMimeType);
        CleanupStack::PushL(videoCodec);

        // Then deep copy QString from that
        videoMimeTypeString = QString::fromUtf16(videoCodec->Ptr(), videoCodec->Length());
        m_videoSettings.setCodec(videoMimeTypeString);

        CleanupStack::PopAndDestroy(videoCodec);
    }

    // Resolution
    TSize symbianResolution;
    TRAP(err, m_videoRecorder->GetVideoFrameSizeL(symbianResolution));
    if (err) {
        if (err != KErrNotSupported)
            setError(err, tr("Querying video resolution failed."));
    }
    QSize resolution = QSize(symbianResolution.iWidth, symbianResolution.iHeight);
    m_videoSettings.setResolution(resolution);

    // FrameRate
    TReal32 frameRate = 0;
    TRAP(err, frameRate = m_videoRecorder->VideoFrameRateL());
    if (err) {
        if (err != KErrNotSupported)
            setError(err, tr("Querying video framerate failed."));
    }
    m_videoSettings.setFrameRate(qreal(frameRate));

    // BitRate
    TInt bitRate = -1;
    TRAP(err, bitRate = m_videoRecorder->VideoBitRateL());
    if (err) {
        if (err != KErrNotSupported)
            setError(err, tr("Querying video bitrate failed."));
    }
    m_videoSettings.setBitRate(int(bitRate));

    // EncodingMode
    m_audioSettings.setEncodingMode(QtMultimediaKit::ConstantQualityEncoding);
}

void S60VideoCaptureSession::videoEncoderSettings(QVideoEncoderSettings &videoSettings)
{
    switch (m_captureState) {
    // CVideoRecorderUtility, return requested settings
    case ENotInitialized:
    case EInitializing:
    case EInitialized:
    case EOpening:
    case EOpenComplete:
    case EPreparing:
        break;

    // Possible to query settings from CVideoRecorderUtility
    case EPrepared:
    case ERecording:
    case EPaused:
        queryVideoEncoderSettings();
        break;

    default:
        videoSettings = QVideoEncoderSettings();
        setError(KErrGeneral, tr("Unexpected video error."));
        return;
    }

    videoSettings = m_videoSettings;
}

void S60VideoCaptureSession::audioEncoderSettings(QAudioEncoderSettings &audioSettings)
{
    switch (m_captureState) {
    // CVideoRecorderUtility, return requested settings
    case ENotInitialized:
    case EInitializing:
    case EInitialized:
    case EOpening:
    case EOpenComplete:
    case EPreparing:
        break;

    // Possible to query settings from CVideoRecorderUtility
    case EPrepared:
    case ERecording:
    case EPaused:
        queryAudioEncoderSettings();
        break;

    default:
        audioSettings = QAudioEncoderSettings();
        setError(KErrGeneral, tr("Unexpected video error."));
        return;
    }

    audioSettings = m_audioSettings;
}

void S60VideoCaptureSession::validateRequestedCodecs()
{
    if (!m_audioCodecList.contains(m_audioSettings.codec())) {
        m_audioSettings.setCodec(KMimeTypeDefaultAudioCodec);
        setError(KErrNotSupported, tr("Currently selected audio codec is not supported by the platform."));
    }
    if (!m_videoCodecList.contains(m_videoSettings.codec())) {
        m_videoSettings.setCodec(KMimeTypeDefaultVideoCodec);
        setError(KErrNotSupported, tr("Currently selected video codec is not supported by the platform."));
    }
}

void S60VideoCaptureSession::setVideoCaptureQuality(const QtMultimediaKit::EncodingQuality quality,
                                                    const VideoQualityDefinition mode)
{
    // Sensible presets
    switch (mode) {
        case ENoVideoQuality:
            // Do nothing
            break;
        case EOnlyVideoQuality:
            if (quality == QtMultimediaKit::VeryLowQuality) {
                m_videoSettings.setResolution(QSize(128,96));
                m_videoSettings.setFrameRate(10);
                m_videoSettings.setBitRate(64000);
            } else if (quality == QtMultimediaKit::LowQuality) {
                m_videoSettings.setResolution(QSize(176,144));
                m_videoSettings.setFrameRate(15);
                m_videoSettings.setBitRate(64000);
            } else if (quality == QtMultimediaKit::NormalQuality) {
                m_videoSettings.setResolution(QSize(176,144));
                m_videoSettings.setFrameRate(15);
                m_videoSettings.setBitRate(128000);
            } else if (quality == QtMultimediaKit::HighQuality) {
                m_videoSettings.setResolution(QSize(352,288));
                m_videoSettings.setFrameRate(15);
                m_videoSettings.setBitRate(384000);
            } else if (quality == QtMultimediaKit::VeryHighQuality) {
                if (m_cameraEngine && m_cameraEngine->CurrentCameraIndex() == 0)
                    m_videoSettings.setResolution(QSize(640,480)); // Primary camera
                else
                    m_videoSettings.setResolution(QSize(352,288)); // Other cameras
                m_videoSettings.setFrameRate(15);
                m_videoSettings.setBitRate(2000000);
            } else {
                m_videoSettings.setQuality(QtMultimediaKit::NormalQuality);
                setError(KErrNotSupported, tr("Unsupported video quality."));
                return;
            }
            break;
        case EVideoQualityAndResolution:
            if (quality == QtMultimediaKit::VeryLowQuality) {
                m_videoSettings.setFrameRate(10);
                m_videoSettings.setBitRate(64000);
            } else if (quality == QtMultimediaKit::LowQuality) {
                m_videoSettings.setFrameRate(15);
                m_videoSettings.setBitRate(64000);
            } else if (quality == QtMultimediaKit::NormalQuality) {
                m_videoSettings.setFrameRate(15);
                m_videoSettings.setBitRate(128000);
            } else if (quality == QtMultimediaKit::HighQuality) {
                m_videoSettings.setFrameRate(15);
                m_videoSettings.setBitRate(384000);
            } else if (quality == QtMultimediaKit::VeryHighQuality) {
                m_videoSettings.setFrameRate(15);
                m_videoSettings.setBitRate(2000000);
            } else {
                m_videoSettings.setQuality(QtMultimediaKit::NormalQuality);
                setError(KErrNotSupported, tr("Unsupported video quality."));
                return;
            }
            break;
        case EVideoQualityAndFrameRate:
            if (quality == QtMultimediaKit::VeryLowQuality) {
                m_videoSettings.setResolution(QSize(128,96));
                m_videoSettings.setBitRate(64000);
            } else if (quality == QtMultimediaKit::LowQuality) {
                m_videoSettings.setResolution(QSize(176,144));
                m_videoSettings.setBitRate(64000);
            } else if (quality == QtMultimediaKit::NormalQuality) {
                m_videoSettings.setResolution(QSize(176,144));
                m_videoSettings.setBitRate(128000);
            } else if (quality == QtMultimediaKit::HighQuality) {
                m_videoSettings.setResolution(QSize(352,288));
                m_videoSettings.setBitRate(384000);
            } else if (quality == QtMultimediaKit::VeryHighQuality) {
                if (m_cameraEngine && m_cameraEngine->CurrentCameraIndex() == 0)
                    m_videoSettings.setResolution(QSize(640,480)); // Primary camera
                else
                    m_videoSettings.setResolution(QSize(352,288)); // Other cameras
                m_videoSettings.setBitRate(2000000);
            } else {
                m_videoSettings.setQuality(QtMultimediaKit::NormalQuality);
                setError(KErrNotSupported, tr("Unsupported video quality."));
                return;
            }
            break;
        case EVideoQualityAndBitRate:
            if (quality == QtMultimediaKit::VeryLowQuality) {
                m_videoSettings.setResolution(QSize(128,96));
                m_videoSettings.setFrameRate(10);
            } else if (quality == QtMultimediaKit::LowQuality) {
                m_videoSettings.setResolution(QSize(176,144));
                m_videoSettings.setFrameRate(15);
            } else if (quality == QtMultimediaKit::NormalQuality) {
                m_videoSettings.setResolution(QSize(176,144));
                m_videoSettings.setFrameRate(15);
            } else if (quality == QtMultimediaKit::HighQuality) {
                m_videoSettings.setResolution(QSize(352,288));
                m_videoSettings.setFrameRate(15);
            } else if (quality == QtMultimediaKit::VeryHighQuality) {
                if (m_cameraEngine && m_cameraEngine->CurrentCameraIndex() == 0)
                    m_videoSettings.setResolution(QSize(640,480)); // Primary camera
                else
                    m_videoSettings.setResolution(QSize(352,288)); // Other cameras
                m_videoSettings.setFrameRate(15);
            } else {
                m_videoSettings.setQuality(QtMultimediaKit::NormalQuality);
                setError(KErrNotSupported, tr("Unsupported video quality."));
                return;
            }
            break;
        case EVideoQualityAndResolutionAndBitRate:
            if (quality == QtMultimediaKit::VeryLowQuality) {
                m_videoSettings.setFrameRate(10);
            } else if (quality == QtMultimediaKit::LowQuality) {
                m_videoSettings.setFrameRate(15);
            } else if (quality == QtMultimediaKit::NormalQuality) {
                m_videoSettings.setFrameRate(15);
            } else if (quality == QtMultimediaKit::HighQuality) {
                m_videoSettings.setFrameRate(15);
            } else if (quality == QtMultimediaKit::VeryHighQuality) {
                m_videoSettings.setFrameRate(15);
            } else {
                m_videoSettings.setQuality(QtMultimediaKit::NormalQuality);
                setError(KErrNotSupported, tr("Unsupported video quality."));
                return;
            }
            break;
        case EVideoQualityAndResolutionAndFrameRate:
            if (quality == QtMultimediaKit::VeryLowQuality) {
                m_videoSettings.setBitRate(64000);
            } else if (quality == QtMultimediaKit::LowQuality) {
                m_videoSettings.setBitRate(64000);
            } else if (quality == QtMultimediaKit::NormalQuality) {
                m_videoSettings.setBitRate(128000);
            } else if (quality == QtMultimediaKit::HighQuality) {
                m_videoSettings.setBitRate(384000);
            } else if (quality == QtMultimediaKit::VeryHighQuality) {
                m_videoSettings.setBitRate(2000000);
            } else {
                m_videoSettings.setQuality(QtMultimediaKit::NormalQuality);
                setError(KErrNotSupported, tr("Unsupported video quality."));
                return;
            }
            break;
        case EVideoQualityAndFrameRateAndBitRate:
            if (quality == QtMultimediaKit::VeryLowQuality) {
                m_videoSettings.setResolution(QSize(128,96));
            } else if (quality == QtMultimediaKit::LowQuality) {
                m_videoSettings.setResolution(QSize(176,144));
            } else if (quality == QtMultimediaKit::NormalQuality) {
                m_videoSettings.setResolution(QSize(176,144));
            } else if (quality == QtMultimediaKit::HighQuality) {
                m_videoSettings.setResolution(QSize(352,288));
            } else if (quality == QtMultimediaKit::VeryHighQuality) {
                if (m_cameraEngine && m_cameraEngine->CurrentCameraIndex() == 0)
                    m_videoSettings.setResolution(QSize(640,480)); // Primary camera
                else
                    m_videoSettings.setResolution(QSize(352,288)); // Other cameras
            } else {
                m_videoSettings.setQuality(QtMultimediaKit::NormalQuality);
                setError(KErrNotSupported, tr("Unsupported video quality."));
                return;
            }
            break;
    }

    m_videoSettings.setQuality(quality);
    m_uncommittedSettings = true;
}

void S60VideoCaptureSession::setAudioCaptureQuality(const QtMultimediaKit::EncodingQuality quality,
                                                    const AudioQualityDefinition mode)
{
    // Based on audio quality definition mode, select proper SampleRate and BitRate
    switch (mode) {
        case EOnlyAudioQuality:
            switch (quality) {
                case QtMultimediaKit::VeryLowQuality:
                    m_audioSettings.setBitRate(16000);
                    m_audioSettings.setSampleRate(-1);
                    break;
                case QtMultimediaKit::LowQuality:
                    m_audioSettings.setBitRate(16000);
                    m_audioSettings.setSampleRate(-1);
                    break;
                case QtMultimediaKit::NormalQuality:
                    m_audioSettings.setBitRate(32000);
                    m_audioSettings.setSampleRate(-1);
                    break;
                case QtMultimediaKit::HighQuality:
                    m_audioSettings.setBitRate(64000);
                    m_audioSettings.setSampleRate(-1);
                    break;
                case QtMultimediaKit::VeryHighQuality:
                    m_audioSettings.setBitRate(64000);
                    m_audioSettings.setSampleRate(-1);
                    break;
                default:
                    m_audioSettings.setQuality(QtMultimediaKit::NormalQuality);
                    setError(KErrNotSupported, tr("Unsupported audio quality."));
                    return;
            }
            break;
        case EAudioQualityAndBitRate:
            switch (quality) {
                case QtMultimediaKit::VeryLowQuality:
                    m_audioSettings.setSampleRate(-1);
                    break;
                case QtMultimediaKit::LowQuality:
                    m_audioSettings.setSampleRate(-1);
                    break;
                case QtMultimediaKit::NormalQuality:
                    m_audioSettings.setSampleRate(-1);
                    break;
                case QtMultimediaKit::HighQuality:
                    m_audioSettings.setSampleRate(-1);
                    break;
                case QtMultimediaKit::VeryHighQuality:
                    m_audioSettings.setSampleRate(-1);
                    break;
                default:
                    m_audioSettings.setQuality(QtMultimediaKit::NormalQuality);
                    setError(KErrNotSupported, tr("Unsupported audio quality."));
                    return;
            }
            break;
        case EAudioQualityAndSampleRate:
            switch (quality) {
                case QtMultimediaKit::VeryLowQuality:
                    m_audioSettings.setBitRate(16000);
                    break;
                case QtMultimediaKit::LowQuality:
                    m_audioSettings.setBitRate(16000);
                    break;
                case QtMultimediaKit::NormalQuality:
                    m_audioSettings.setBitRate(32000);
                    break;
                case QtMultimediaKit::HighQuality:
                    m_audioSettings.setBitRate(64000);
                    break;
                case QtMultimediaKit::VeryHighQuality:
                    m_audioSettings.setBitRate(64000);
                    break;
                default:
                    m_audioSettings.setQuality(QtMultimediaKit::NormalQuality);
                    setError(KErrNotSupported, tr("Unsupported audio quality."));
                    return;
            }
            break;
        case ENoAudioQuality:
            // No actions required, just set quality parameter
            break;

        default:
            setError(KErrGeneral, tr("Unexpected camera error."));
            return;
    }

    m_audioSettings.setQuality(quality);
    m_uncommittedSettings = true;
}

int S60VideoCaptureSession::initializeVideoRecording()
{
    if (m_error)
        return m_error;

    TRAPD(symbianError, doInitializeVideoRecorderL());
    setError(symbianError, tr("Failed to initialize video recorder."));

    return symbianError;
}

void S60VideoCaptureSession::releaseVideoRecording()
{
    if (m_captureState >= ERecording) {
        m_videoRecorder->Stop();
        if (m_durationTimer->isActive())
            m_durationTimer->stop();
    }

    if (m_captureState >= EInitialized)
        m_videoRecorder->Close();

    // Reset state
    m_captureState = ENotInitialized;

    // Reset error to be able to recover from error
    m_error = KErrNone;

    // Reset flags
    m_openWhenReady = false;
    m_prepareAfterOpenComplete = false;
    m_startAfterPrepareComplete = false;
    m_uncommittedSettings = false;
    m_commitSettingsWhenReady = false;
}

void S60VideoCaptureSession::startRecording()
{
    if (m_error) {
        setError(m_error, tr("Unexpected recording error."));
        return;
    }

    switch (m_captureState) {
        case ENotInitialized:
        case EInitializing:
        case EInitialized:
            if (m_captureState == EInitialized)
                setOutputLocation(m_requestedSink);
            m_startAfterPrepareComplete = true;
            return;

        case EOpening:
        case EPreparing:
            // Execute FileOpenL() and Prepare() asap and then start recording
            m_startAfterPrepareComplete = true;
            return;
        case EOpenComplete:
        case EPrepared:
            if (m_captureState == EPrepared && !m_uncommittedSettings)
                break;

            // Revert state internally, since logically applying settings means going
            // from OpenComplete ==> Preparing ==> Prepared.
            m_captureState = EOpenComplete;
            m_startAfterPrepareComplete = true;

            // Commit settings and prepare with them
            applyAllSettings();
            return;
        case ERecording:
            // Discard
            return;
        case EPaused:
            // Continue
            break;

        default:
            setError(KErrGeneral, tr("Unexpected camera error."));
            return;
    }

    // State should now be either Prepared with no Uncommitted Settings or Paused

    if (!m_cameraStarted) {
        m_startAfterPrepareComplete = true;
        return;
    }

    if (m_cameraEngine && !m_cameraEngine->IsCameraReady()) {
        setError(KErrNotReady, tr("Camera not ready to start video recording."));
        return;
    }

    if (m_videoRecorder) {
        m_videoRecorder->Record();
        m_captureState = ERecording;
        emit stateChanged(m_captureState);
        m_durationTimer->start();

        // Reset all flags
        m_openWhenReady = false;
        m_prepareAfterOpenComplete = false;
        m_startAfterPrepareComplete = false;
    } else {
        setError(KErrNotReady, tr("Unexpected camera error."));
    }
}

void S60VideoCaptureSession::pauseRecording()
{
    if (m_captureState == ERecording) {
        if (m_videoRecorder) {
            TRAPD(err, m_videoRecorder->PauseL());
            setError(err, tr("Pausing video recording failed."));
            m_captureState = EPaused;
            emit stateChanged(m_captureState);
            if (m_durationTimer->isActive())
                m_durationTimer->stop();

            // Notify last duration
            TRAP(err, m_position = m_videoRecorder->DurationL().Int64() / 1000);
            setError(err, tr("Cannot retrieve video position."));
            emit positionChanged(m_position);
        }
        else
            setError(KErrNotReady, tr("Unexpected camera error."));
    }
}

void S60VideoCaptureSession::stopRecording(const bool reInitialize)
{
    if (m_captureState != ERecording && m_captureState != EPaused)
        return; // Ignore

    if (m_videoRecorder) {
        m_videoRecorder->Stop();
        m_videoRecorder->Close();

        // Notify muting is disabled if needed
        if (m_muted)
            emit mutedChanged(false);

        m_captureState = ENotInitialized;
        emit stateChanged(m_captureState);

        if (m_durationTimer->isActive())
            m_durationTimer->stop();

        // VideoRecording will be re-initialized unless explicitly requested not to do so
        if (reInitialize) {
            if (m_cameraEngine->IsCameraReady())
                initializeVideoRecording();
        }
    }
    else
        setError(KErrNotReady, tr("Unexpected camera error."));
}

void S60VideoCaptureSession::updateVideoCaptureContainers()
{
    TRAPD(err, doUpdateVideoCaptureContainersL());
    setError(err, tr("Failed to gather video container information."));
}

void S60VideoCaptureSession::doUpdateVideoCaptureContainersL()
{
    // Clear container data structure
    QList<TInt> mapControllers = m_videoControllerMap.keys();
    for (int i = 0; i < mapControllers.size(); ++i) {
        foreach(VideoFormatData data, m_videoControllerMap[mapControllers[i]]){
            data.supportedMimeTypes.clear();
        }
        m_videoControllerMap[mapControllers[i]].clear();
    }
    m_videoControllerMap.clear();

    // Resolve the supported video format and retrieve a list of controllers
    CMMFControllerPluginSelectionParameters* pluginParameters =
        CMMFControllerPluginSelectionParameters::NewLC();
    CMMFFormatSelectionParameters* format =
        CMMFFormatSelectionParameters::NewLC();

    // Set the play and record format selection parameters to be blank.
    // Format support is only retrieved if requested.
    pluginParameters->SetRequiredPlayFormatSupportL(*format);
    pluginParameters->SetRequiredRecordFormatSupportL(*format);

    // Set the media IDs
    RArray<TUid> mediaIds;
    CleanupClosePushL(mediaIds);

    User::LeaveIfError(mediaIds.Append(KUidMediaTypeVideo));

    // Get plugins that support at least video
    pluginParameters->SetMediaIdsL(mediaIds,
        CMMFPluginSelectionParameters::EAllowOtherMediaIds);
    pluginParameters->SetPreferredSupplierL(KNullDesC,
        CMMFPluginSelectionParameters::EPreferredSupplierPluginsFirstInList);

    // Array to hold all the controllers support the match data
    RMMFControllerImplInfoArray controllers;
    CleanupResetAndDestroyPushL(controllers);
    pluginParameters->ListImplementationsL(controllers);

    // Find the first controller with at least one record format available
    for (TInt index = 0; index < controllers.Count(); ++index) {

        m_videoControllerMap.insert(controllers[index]->Uid().iUid, QHash<TInt,VideoFormatData>());

        const RMMFFormatImplInfoArray& recordFormats = controllers[index]->RecordFormats();
        for (TInt j = 0; j < recordFormats.Count(); ++j) {
            VideoFormatData formatData;
            formatData.description = QString::fromUtf16(
                                    recordFormats[j]->DisplayName().Ptr(),
                                    recordFormats[j]->DisplayName().Length());

            const CDesC8Array& mimeTypes = recordFormats[j]->SupportedMimeTypes();
            for (int k = 0; k < mimeTypes.Count(); ++k) {
                TPtrC8 mimeType = mimeTypes[k];
                QString type = QString::fromUtf8((char *)mimeType.Ptr(),
                        mimeType.Length());
                formatData.supportedMimeTypes.append(type);
            }

            m_videoControllerMap[controllers[index]->Uid().iUid].insert(recordFormats[j]->Uid().iUid, formatData);
        }
    }

    CleanupStack::PopAndDestroy(&controllers);
    CleanupStack::PopAndDestroy(&mediaIds);
    CleanupStack::PopAndDestroy(format);
    CleanupStack::PopAndDestroy(pluginParameters);
}

/*
 * This goes through the available controllers and selects proper one based
 * on the format. Function sets proper UIDs to be used for controller and format.
 */
void S60VideoCaptureSession::selectController(const QString &format,
                                              TUid &controllerUid,
                                              TUid &formatUid)
{
    QList<TInt> controllers = m_videoControllerMap.keys();
    QList<TInt> formats;

    for (int i = 0; i < controllers.count(); ++i) {
        formats = m_videoControllerMap[controllers[i]].keys();
        for (int j = 0; j < formats.count(); ++j) {
            VideoFormatData formatData = m_videoControllerMap[controllers[i]][formats[j]];
            if (formatData.supportedMimeTypes.contains(format, Qt::CaseInsensitive)) {
                controllerUid = TUid::Uid(controllers[i]);
                formatUid = TUid::Uid(formats[j]);
            }
        }
    }
}

QStringList S60VideoCaptureSession::supportedVideoCaptureCodecs()
{
    return m_videoCodecList;
}

QStringList S60VideoCaptureSession::supportedAudioCaptureCodecs()
{
    QStringList keys = m_audioCodecList.keys();
    keys.sort();
    return keys;
}

QList<int> S60VideoCaptureSession::supportedSampleRates(const QAudioEncoderSettings &settings, bool *continuous)
{
    QList<int> rates;

    TRAPD(err, rates = doGetSupportedSampleRatesL(settings, continuous));
    if (err != KErrNotSupported)
        setError(err, tr("Failed to query information of supported sample rates."));

    return rates;
}

QList<int> S60VideoCaptureSession::doGetSupportedSampleRatesL(const QAudioEncoderSettings &settings, bool *continuous)
{
    QList<int> sampleRates;

    if (m_captureState < EOpenComplete)
        return sampleRates;

#ifndef S60_31_PLATFORM
    RArray<TUint> supportedSampleRates;
    CleanupClosePushL(supportedSampleRates);

    if (!settings.codec().isEmpty()) {

        TFourCC currentAudioCodec;
        currentAudioCodec = m_videoRecorder->AudioTypeL();

        TFourCC requestedAudioCodec;
        if (qstrcmp(settings.codec().toLocal8Bit().constData(), "audio/aac") == 0)
            requestedAudioCodec.Set(KMMFFourCCCodeAAC);
        else if (qstrcmp(settings.codec().toLocal8Bit().constData(), "audio/amr") == 0)
            requestedAudioCodec.Set(KMMFFourCCCodeAMR);
        m_videoRecorder->SetAudioTypeL(requestedAudioCodec);

        m_videoRecorder->GetSupportedAudioSampleRatesL(supportedSampleRates);

        m_videoRecorder->SetAudioTypeL(currentAudioCodec);
    }
    else
        m_videoRecorder->GetSupportedAudioSampleRatesL(supportedSampleRates);

    for (int i = 0; i < supportedSampleRates.Count(); ++i)
        sampleRates.append(int(supportedSampleRates[i]));

    CleanupStack::PopAndDestroy(); // RArray<TUint> supportedSampleRates
#else // S60 3.1 Platform
    Q_UNUSED(settings);
#endif // S60_31_PLATFORM

    if (continuous)
        *continuous = false;

    return sampleRates;
}

void S60VideoCaptureSession::setAudioSampleRate(const int sampleRate)
{
    if (sampleRate != -1)
        m_audioSettings.setSampleRate(sampleRate);

    m_uncommittedSettings = true;
}

void S60VideoCaptureSession::setAudioBitRate(const int bitRate)
{
    if (bitRate != -1)
        m_audioSettings.setBitRate(bitRate);

    m_uncommittedSettings = true;
}

void S60VideoCaptureSession::setAudioChannelCount(const int channelCount)
{
    if (channelCount != -1)
        m_audioSettings.setChannelCount(channelCount);

    m_uncommittedSettings = true;
}

void S60VideoCaptureSession::setVideoCaptureCodec(const QString &codecName)
{
    if (codecName == m_videoSettings.codec())
        return;

    if (codecName.isEmpty())
        m_videoSettings.setCodec(KMimeTypeDefaultVideoCodec); // Use default
    else
        m_videoSettings.setCodec(codecName);

    m_uncommittedSettings = true;
}

void S60VideoCaptureSession::setAudioCaptureCodec(const QString &codecName)
{
    if (codecName == m_audioSettings.codec())
        return;

    if (codecName.isEmpty()) {
        m_audioSettings.setCodec(KMimeTypeDefaultAudioCodec); // Use default
    } else {
        // If information of supported codecs is already available check that
        // given codec is supported
        if (m_captureState >= EOpenComplete) {
            if (m_audioCodecList.contains(codecName)) {
                m_audioSettings.setCodec(codecName);
                m_uncommittedSettings = true;
            } else {
                setError(KErrNotSupported, tr("Requested audio codec is not supported"));
            }
        } else {
            m_audioSettings.setCodec(codecName);
            m_uncommittedSettings = true;
        }
    }
}

QString S60VideoCaptureSession::videoCaptureCodecDescription(const QString &codecName)
{
    QString codecDescription;
    if (codecName.contains("video/H263-2000", Qt::CaseInsensitive))
        codecDescription.append("H.263 Video Codec");
    else if (codecName.contains("video/mp4v-es", Qt::CaseInsensitive))
        codecDescription.append("MPEG-4 Part 2 Video Codec");
    else if (codecName.contains("video/H264", Qt::CaseInsensitive))
        codecDescription.append("H.264 AVC (MPEG-4 Part 10) Video Codec");
    else
        codecDescription.append("Video Codec");

    return codecDescription;
}

void S60VideoCaptureSession::doSetCodecsL()
{
    // Determine Profile and Level for the video codec if needed
    // (MimeType/Profile-level-id contains "profile" if profile/level info is available)
    if (!m_videoSettings.codec().contains(QString("profile"), Qt::CaseInsensitive))
        m_videoSettings.setCodec(determineProfileAndLevel());

    if (m_videoRecorder) {
        TPtrC16 str(reinterpret_cast<const TUint16*>(m_videoSettings.codec().utf16()));
        HBufC8* videoCodec(0);
        videoCodec = CnvUtfConverter::ConvertFromUnicodeToUtf8L(str);
        CleanupStack::PushL(videoCodec);

        TFourCC audioCodec = m_audioCodecList[m_audioSettings.codec()];

        TInt vErr = KErrNone;
        TInt aErr = KErrNone;
        TRAP(vErr, m_videoRecorder->SetVideoTypeL(*videoCodec));
        TRAP(aErr, m_videoRecorder->SetAudioTypeL(audioCodec));

        User::LeaveIfError(vErr);
        User::LeaveIfError(aErr);

        CleanupStack::PopAndDestroy(videoCodec);
    }
    else
        setError(KErrNotReady, tr("Unexpected camera error."));
}

QString S60VideoCaptureSession::determineProfileAndLevel()
{
    QString determinedMimeType = m_videoSettings.codec();

    // H.263
    if (determinedMimeType.contains(QString("video/H263-2000"), Qt::CaseInsensitive)) {
        if ((m_videoSettings.resolution().width() * m_videoSettings.resolution().height()) > (176*144)) {
            if (m_videoSettings.frameRate() > 15.0)
                determinedMimeType.append("; profile=0; level=20");
            else
                determinedMimeType.append("; profile=0; level=40");
        } else {
            if (m_videoSettings.bitRate() > 64000)
                determinedMimeType.append("; profile=0; level=45");
            else
                determinedMimeType.append("; profile=0; level=10");
        }

    // MPEG-4
    } else if (determinedMimeType.contains(QString("video/mp4v-es"), Qt::CaseInsensitive)) {
        if ((m_videoSettings.resolution().width() * m_videoSettings.resolution().height()) > (720*480)) {
            determinedMimeType.append("; profile-level-id=6");
        } else if ((m_videoSettings.resolution().width() * m_videoSettings.resolution().height()) > (640*480)) {
            determinedMimeType.append("; profile-level-id=5");
        } else if ((m_videoSettings.resolution().width() * m_videoSettings.resolution().height()) > (352*288)) {
            determinedMimeType.append("; profile-level-id=4");
        } else if ((m_videoSettings.resolution().width() * m_videoSettings.resolution().height()) > (176*144)) {
            if (m_videoSettings.frameRate() > 15.0)
                determinedMimeType.append("; profile-level-id=3");
            else
                determinedMimeType.append("; profile-level-id=2");
        } else {
            if (m_videoSettings.bitRate() > 64000)
                determinedMimeType.append("; profile-level-id=9");
            else
                determinedMimeType.append("; profile-level-id=1");
        }

    // H.264
    } else if (determinedMimeType.contains(QString("video/H264"), Qt::CaseInsensitive)) {
        if ((m_videoSettings.resolution().width() * m_videoSettings.resolution().height()) > (640*480)) {
            determinedMimeType.append("; profile-level-id=42801F");
        } else if ((m_videoSettings.resolution().width() * m_videoSettings.resolution().height()) > (352*288)) {
            determinedMimeType.append("; profile-level-id=42801E");
        } else if ((m_videoSettings.resolution().width() * m_videoSettings.resolution().height()) > (176*144)) {
            if (m_videoSettings.frameRate() > 15.0)
                determinedMimeType.append("; profile-level-id=428015");
            else
                determinedMimeType.append("; profile-level-id=42800C");
        } else {
            determinedMimeType.append("; profile-level-id=42900B");
        }
    }

    return determinedMimeType;
}

void S60VideoCaptureSession::setBitrate(const int bitrate)
{
    m_videoSettings.setBitRate(bitrate);

    m_uncommittedSettings = true;
}

void S60VideoCaptureSession::doSetBitrate(const int &bitrate)
{
    if (bitrate != -1) {
        if (m_videoRecorder) {
            TRAPD(err, m_videoRecorder->SetVideoBitRateL(bitrate));
            if (err) {
                if (err == KErrNotSupported || err == KErrArgument) {
                    setError(KErrNotSupported, tr("Requested video bitrate is not supported."));
                    m_videoSettings.setBitRate(64000); // Reset
                } else {
                    setError(err, tr("Failed to set video bitrate."));
                }
            }
        } else {
            setError(KErrNotReady, tr("Unexpected camera error."));
        }
    }
}

void S60VideoCaptureSession::setVideoResolution(const QSize &resolution)
{
    m_videoSettings.setResolution(resolution);

    m_uncommittedSettings = true;
}

void S60VideoCaptureSession::doSetVideoResolution(const QSize &resolution)
{
    TSize size((TInt)resolution.width(), (TInt)resolution.height());

    // Make sure resolution is not too big if main camera is not used
    if (m_cameraEngine->CurrentCameraIndex() != 0) {
        TCameraInfo *info = m_cameraEngine->CameraInfo();
        if (info) {
            TInt videoResolutionCount = info->iNumVideoFrameSizesSupported;
            TSize maxCameraVideoResolution = TSize(0,0);
            CCamera *camera = m_cameraEngine->Camera();
            if (camera) {
                for (TInt i = 0; i < videoResolutionCount; ++i) {
                    TSize checkedResolution;
                    // Use YUV video max frame size in the check (Through
                    // CVideoRecorderUtility/DevVideoRecord it is possible to
                    // query only encoder maximums)
                    camera->EnumerateVideoFrameSizes(checkedResolution, i, CCamera::EFormatYUV420Planar);
                    if ((checkedResolution.iWidth * checkedResolution.iHeight) >
                        (maxCameraVideoResolution.iWidth * maxCameraVideoResolution.iHeight))
                        maxCameraVideoResolution = checkedResolution;
                }
                if ((maxCameraVideoResolution.iWidth * maxCameraVideoResolution.iHeight) <
                    (size.iWidth * size.iHeight)) {
                    size = maxCameraVideoResolution;
                    setError(KErrNotSupported, tr("Requested resolution is not supported for this camera."));
                }
            }
            else
                setError(KErrGeneral, tr("Could not query supported video resolutions."));
        }else
            setError(KErrGeneral, tr("Could not query supported video resolutions."));
    }

    if (resolution.width() != -1 && resolution.height() != -1) {
        if (m_videoRecorder) {
            TRAPD(err, m_videoRecorder->SetVideoFrameSizeL((TSize)size));
            if (err == KErrNotSupported || err == KErrArgument) {
                setError(KErrNotSupported, tr("Requested video resolution is not supported."));
                TSize fallBack(640,480);
                TRAPD(err, m_videoRecorder->SetVideoFrameSizeL(fallBack));
                if (err == KErrNone) {
                    m_videoSettings.setResolution(QSize(fallBack.iWidth,fallBack.iHeight));
                } else {
                    fallBack = TSize(176,144);
                    TRAPD(err, m_videoRecorder->SetVideoFrameSizeL(fallBack));
                    if (err == KErrNone)
                        m_videoSettings.setResolution(QSize(fallBack.iWidth,fallBack.iHeight));
                }
            } else {
                setError(err, tr("Failed to set video resolution."));
            }
        } else {
            setError(KErrNotReady, tr("Unexpected camera error."));
        }
    }
}

void S60VideoCaptureSession::setFrameRate(qreal rate)
{
    m_videoSettings.setFrameRate(rate);

    m_uncommittedSettings = true;
}

void S60VideoCaptureSession::doSetFrameRate(qreal rate)
{
    if (rate != 0) {
        if (m_videoRecorder) {
            bool continuous = false;
            QList<qreal> list = supportedVideoFrameRates(&continuous);
            qreal maxRate = 0.0;
            foreach (qreal fRate, list)
                if (fRate > maxRate)
                    maxRate = fRate;
            if (maxRate >= rate && rate > 0) {
                TRAPD(err, m_videoRecorder->SetVideoFrameRateL((TReal32)rate));
                if (err == KErrNotSupported) {
                    setError(KErrNotSupported, tr("Requested framerate is not supported."));
                    TReal32 fallBack = 15.0;
                    TRAPD(err, m_videoRecorder->SetVideoFrameRateL(fallBack));
                    if (err == KErrNone)
                        m_videoSettings.setFrameRate((qreal)fallBack);
                } else {
                    if (err == KErrArgument) {
                        setError(KErrNotSupported, tr("Requested framerate is not supported."));
                        m_videoSettings.setFrameRate(15.0); // Reset
                    } else {
                        setError(err, tr("Failed to set video framerate."));
                    }
                }
            } else {
                setError(KErrNotSupported, tr("Requested framerate is not supported."));
                m_videoSettings.setFrameRate(15.0); // Reset
            }
        } else {
            setError(KErrNotReady, tr("Unexpected camera error."));
        }
    }
}

void S60VideoCaptureSession::setVideoEncodingMode(const QtMultimediaKit::EncodingMode mode)
{
    // This has no effect as it has no support in Symbian

    if (mode == QtMultimediaKit::ConstantQualityEncoding) {
        m_videoSettings.setEncodingMode(mode);
        return;
    }

    setError(KErrNotSupported, tr("Requested video encoding mode is not supported"));

    // m_uncommittedSettings = true;
}

void S60VideoCaptureSession::setAudioEncodingMode(const QtMultimediaKit::EncodingMode mode)
{
    // This has no effect as it has no support in Symbian

    if (mode == QtMultimediaKit::ConstantQualityEncoding) {
        m_audioSettings.setEncodingMode(mode);
        return;
    }

    setError(KErrNotSupported, tr("Requested audio encoding mode is not supported"));

    // m_uncommittedSettings = true;
}

void S60VideoCaptureSession::initializeVideoCaptureSettings()
{
    // Check if user has already requested some settings
    if (m_captureSettingsSet)
        return;

    QSize resolution(-1, -1);
    qreal frameRate(0);
    int bitRate(-1);

    if (m_cameraEngine) {

        if (m_videoRecorder && m_captureState >= EInitialized) {

            // Resolution
            QList<QSize> resos = supportedVideoResolutions(0);
            foreach (QSize reso, resos) {
                if ((reso.width() * reso.height()) > (resolution.width() * resolution.height()))
                    resolution = reso;
            }

            // Needed to query supported framerates for this codec/resolution pair
            m_videoSettings.setCodec(KMimeTypeDefaultVideoCodec);
            m_videoSettings.setResolution(resolution);

            // FrameRate
            QList<qreal> fRates = supportedVideoFrameRates(m_videoSettings, 0);
            foreach (qreal rate, fRates) {
                if (rate > frameRate)
                    frameRate = rate;
            }

            // BitRate
#ifdef SYMBIAN_3_PLATFORM
            if (m_cameraEngine->CurrentCameraIndex() == 0)
                bitRate = KBiR_H264_PLID_42801F    // 14Mbps
            else
                bitRate = KBiR_H264_PLID_428016    // 4Mbps
#else // Other platforms
            if (m_cameraEngine->CurrentCameraIndex() == 0)
                bitRate = KBiR_MPEG4_PLID_4        // 2/4Mbps
            else
                bitRate = KBiR_MPEG4_PLID_3        // 384kbps
#endif // SYMBIAN_3_PLATFORM

        } else {
#ifdef SYMBIAN_3_PLATFORM
            if (m_cameraEngine->CurrentCameraIndex() == 0) {
                // Primary camera
                resolution = KResH264_PLID_42801F;  // 1280x720
                frameRate = KFrR_H264_PLID_42801F;  // 30fps
                bitRate = KBiR_H264_PLID_42801F;    // 14Mbps
            } else {
                // Other cameras
                resolution = KResH264_PLID_42801E;  // 640x480
                frameRate = KFrR_H264_PLID_428014;  // 30fps
                bitRate = KBiR_H264_PLID_428016;    // 4Mbps
            }
#else // Other platforms
            if (m_cameraEngine->CurrentCameraIndex() == 0) {
                // Primary camera
                resolution = KResMPEG4_PLID_4;      // 640x480
                frameRate = KFrR_MPEG4_PLID_4;      // 15/30fps
                bitRate = KBiR_MPEG4_PLID_4;        // 2/4Mbps
            } else {
                // Other cameras
                resolution = KResMPEG4_PLID_3;      // 352x288
                frameRate = KFrR_MPEG4;             // 15fps
                bitRate = KBiR_MPEG4_PLID_3;        // 384kbps
            }
#endif // SYMBIAN_3_PLATFORM
        }
    } else {
#ifdef SYMBIAN_3_PLATFORM
        resolution = KResH264_PLID_42801F;
        frameRate = KFrR_H264_PLID_42801F;
        bitRate = KBiR_H264_PLID_42801F;
#else // Pre-Symbian3 Platforms
        resolution = KResMPEG4_PLID_4;
        frameRate = KFrR_MPEG4_PLID_4;
        bitRate = KBiR_MPEG4_PLID_4;
#endif // SYMBIAN_3_PLATFORM
    }

    // Set specified settings (Resolution, FrameRate and BitRate)
    m_videoSettings.setResolution(resolution);
    m_videoSettings.setFrameRate(frameRate);
    m_videoSettings.setBitRate(bitRate);

    // Video Settings: Codec, EncodingMode and Quality
    m_videoSettings.setCodec(KMimeTypeDefaultVideoCodec);
    m_videoSettings.setEncodingMode(QtMultimediaKit::ConstantQualityEncoding);
    m_videoSettings.setQuality(QtMultimediaKit::VeryHighQuality);

    // Audio Settings
    m_audioSettings.setCodec(KMimeTypeDefaultAudioCodec);
    m_audioSettings.setBitRate(KDefaultBitRate);
    m_audioSettings.setSampleRate(KDefaultSampleRate);
    m_audioSettings.setChannelCount(KDefaultChannelCount);
    m_audioSettings.setEncodingMode(QtMultimediaKit::ConstantQualityEncoding);
    m_audioSettings.setQuality(QtMultimediaKit::VeryHighQuality);
}

QSize S60VideoCaptureSession::pixelAspectRatio()
{
#ifndef S60_31_PLATFORM
    TVideoAspectRatio par;
    TRAPD(err, m_videoRecorder->GetPixelAspectRatioL(par));
    if (err)
        setError(err, tr("Failed to query current pixel aspect ratio."));
    return QSize(par.iNumerator, par.iDenominator);
#else // S60_31_PLATFORM
    return QSize();
#endif // !S60_31_PLATFORM
}

void S60VideoCaptureSession::setPixelAspectRatio(const QSize par)
{
#ifndef S60_31_PLATFORM

    const TVideoAspectRatio videoPar(par.width(), par.height());
    TRAPD(err, m_videoRecorder->SetPixelAspectRatioL(videoPar));
    if (err)
        setError(err, tr("Failed to set pixel aspect ratio."));
#else // S60_31_PLATFORM
    Q_UNUSED(par);
#endif // !S60_31_PLATFORM

    m_uncommittedSettings = true;
}

int S60VideoCaptureSession::gain()
{
    TInt gain = 0;
    TRAPD(err, gain = m_videoRecorder->GainL());
    if (err)
        setError(err, tr("Failed to query video gain."));
    return (int)gain;
}

void S60VideoCaptureSession::setGain(const int gain)
{
    TRAPD(err, m_videoRecorder->SetGainL(gain));
    if (err)
        setError(err, tr("Failed to set video gain."));

    m_uncommittedSettings = true;
}

int S60VideoCaptureSession::maxClipSizeInBytes() const
{
    return m_maxClipSize;
}

void S60VideoCaptureSession::setMaxClipSizeInBytes(const int size)
{
    TRAPD(err, m_videoRecorder->SetMaxClipSizeL(size));
    if (err) {
        setError(err, tr("Failed to set maximum video size."));
    } else
        m_maxClipSize = size;

    m_uncommittedSettings = true;
}

void S60VideoCaptureSession::MvruoOpenComplete(TInt aError)
{
    if (m_error)
        return;

    if (aError == KErrNone && m_videoRecorder) {
        if (m_captureState == EInitializing) {
            // Dummy file open completed, initialize settings
            TRAPD(err, doPopulateAudioCodecsL());
            setError(err, tr("Failed to gather information of supported audio codecs."));

            // For DevVideoRecord codecs are populated during
            // doPopulateVideoCodecsDataL()
            TRAP(err, doPopulateVideoCodecsL());
            setError(err, tr("Failed to gather information of supported video codecs."));
#ifndef S60_DEVVIDEO_RECORDING_SUPPORTED
            // Max parameters needed to be populated, if not using DevVideoRecord
            // Otherwise done already in constructor
            doPopulateMaxVideoParameters();
#endif // S60_DEVVIDEO_RECORDING_SUPPORTED

            m_captureState = EInitialized;
            emit stateChanged(m_captureState);

            // Initialize settings if not already done
            initializeVideoCaptureSettings();

            // Validate codecs to be used
            validateRequestedCodecs();

            if (m_openWhenReady || m_prepareAfterOpenComplete || m_startAfterPrepareComplete) {
                setOutputLocation(m_requestedSink);
                m_openWhenReady = false; // Reset
            }
            if (m_commitSettingsWhenReady) {
                applyAllSettings();
                m_commitSettingsWhenReady = false; // Reset
            }
            return;

        } else if (m_captureState == EOpening) {
            // Actual file open completed
            m_captureState = EOpenComplete;
            emit stateChanged(m_captureState);

            // Prepare right away
            if (m_startAfterPrepareComplete || m_prepareAfterOpenComplete) {
                m_prepareAfterOpenComplete = false; // Reset

                // Commit settings and prepare with them
                applyAllSettings();
            }
            return;

        } else if (m_captureState == ENotInitialized) {
            // Resources released while waiting OpenFileL to complete
            m_videoRecorder->Close();
            return;

        } else {
            setError(KErrGeneral, tr("Unexpected camera error."));
            return;
        }
    }

    m_videoRecorder->Close();
    if (aError == KErrNotFound || aError == KErrNotSupported || aError == KErrArgument)
        setError(KErrGeneral, tr("Requested video container or controller is not supported."));
    else
        setError(KErrGeneral, tr("Failure during video recorder initialization."));
}

void S60VideoCaptureSession::MvruoPrepareComplete(TInt aError)
{
    if (m_error)
        return;

    if(aError == KErrNone) {
        if (m_captureState == ENotInitialized) {
            // Resources released while waiting for Prepare to complete
            m_videoRecorder->Close();
            return;
        }

        emit captureSizeChanged(m_videoSettings.resolution());

        m_captureState = EPrepared;
        emit stateChanged(EPrepared);

        // Check the actual active settings
        queryAudioEncoderSettings();
        queryVideoEncoderSettings();

        if (m_openWhenReady == true) {
            setOutputLocation(m_requestedSink);
            m_openWhenReady = false; // Reset
        }

        if (m_commitSettingsWhenReady) {
            applyAllSettings();
            m_commitSettingsWhenReady = false; // Reset
        }

        if (m_startAfterPrepareComplete) {
            m_startAfterPrepareComplete = false; // Reset
            startRecording();
        }
    } else {
        m_videoRecorder->Close();
        if (aError == KErrNotSupported)
            setError(aError, tr("Camera preparation for video recording failed because of unsupported setting."));
        else
            setError(aError, tr("Failed to prepare camera for video recording."));
    }
}

void S60VideoCaptureSession::MvruoRecordComplete(TInt aError)
{
    if (!m_videoRecorder) {
        setError(KErrNotReady, tr("Unexpected camera error."));
        return;
    }

    if((aError == KErrNone || aError == KErrCompletion)) {
        m_videoRecorder->Stop();

        // Reset state
        if (m_captureState != ENotInitialized) {
            m_captureState = ENotInitialized;
            emit stateChanged(m_captureState);
            if (m_durationTimer->isActive())
                m_durationTimer->stop();
        }

        if (m_cameraEngine->IsCameraReady())
            initializeVideoRecording();
    }
    m_videoRecorder->Close();

    // Notify muting is disabled if needed
    if (m_muted)
        emit mutedChanged(false);

    if (aError == KErrDiskFull)
        setError(aError, tr("Not enough space for video, recording stopped."));
    else
        setError(aError, tr("Recording stopped due to unexpected error."));
}

void S60VideoCaptureSession::MvruoEvent(const TMMFEvent& aEvent)
{
    Q_UNUSED(aEvent);
}

#ifdef S60_DEVVIDEO_RECORDING_SUPPORTED
void S60VideoCaptureSession::MdvroReturnPicture(TVideoPicture *aPicture)
{
    // Not used
    Q_UNUSED(aPicture);
}

void S60VideoCaptureSession::MdvroSupplementalInfoSent()
{
    // Not used
}

void S60VideoCaptureSession::MdvroNewBuffers()
{
    // Not used
}

void S60VideoCaptureSession::MdvroFatalError(TInt aError)
{
    setError(aError, tr("Unexpected camera error."));
}

void S60VideoCaptureSession::MdvroInitializeComplete(TInt aError)
{
    // Not used
    Q_UNUSED(aError);
}

void S60VideoCaptureSession::MdvroStreamEnd()
{
    // Not used
}

/*
 * This populates video codec information (supported codecs, resolutions,
 * framerates, etc.) using DevVideoRecord API.
 */
void S60VideoCaptureSession::doPopulateVideoCodecsDataL()
{
    RArray<TUid> encoders;
    CleanupClosePushL(encoders);

    CMMFDevVideoRecord *mDevVideoRecord = CMMFDevVideoRecord::NewL(*this);
    CleanupStack::PushL(mDevVideoRecord);

    // Retrieve list of all encoders provided by the platform
    mDevVideoRecord->GetEncoderListL(encoders);

    for (int i = 0; i < encoders.Count(); ++i ) {

        CVideoEncoderInfo *encoderInfo = mDevVideoRecord->VideoEncoderInfoLC(encoders[i]);

        // Discard encoders that are not HW accelerated and do not support direct capture
        if (encoderInfo->Accelerated() == false || encoderInfo->SupportsDirectCapture() == false) {
            CleanupStack::Check(encoderInfo);
            CleanupStack::PopAndDestroy(encoderInfo);
            continue;
        }

        m_videoParametersForEncoder.append(MaxResolutionRatesAndTypes());
        int newIndex = m_videoParametersForEncoder.count() - 1;

        m_videoParametersForEncoder[newIndex].bitRate = (int)encoderInfo->MaxBitrate();

        // Get supported MIME Types
        const RPointerArray<CCompressedVideoFormat> &videoFormats = encoderInfo->SupportedOutputFormats();
        for(int x = 0; x < videoFormats.Count(); ++x) {
            QString codecMimeType = QString::fromUtf8((char *)videoFormats[x]->MimeType().Ptr(),videoFormats[x]->MimeType().Length());

            m_videoParametersForEncoder[newIndex].mimeTypes.append(codecMimeType);
        }

        // Get supported maximum Resolution/Framerate pairs
        const RArray<TPictureRateAndSize> &ratesAndSizes = encoderInfo->MaxPictureRates();
        SupportedFrameRatePictureSize data;
        for(int j = 0; j < ratesAndSizes.Count(); ++j) {
            data.frameRate = ratesAndSizes[j].iPictureRate;
            data.frameSize = QSize(ratesAndSizes[j].iPictureSize.iWidth, ratesAndSizes[j].iPictureSize.iHeight);

            // Save data to the hash
            m_videoParametersForEncoder[newIndex].frameRatePictureSizePair.append(data);
        }

        CleanupStack::Check(encoderInfo);
        CleanupStack::PopAndDestroy(encoderInfo);
    }

    CleanupStack::Check(mDevVideoRecord);
    CleanupStack::PopAndDestroy(mDevVideoRecord);
    CleanupStack::PopAndDestroy(); // RArray<TUid> encoders
}
#endif  // S60_DEVVIDEO_RECORDING_SUPPORTED

QStringList S60VideoCaptureSession::supportedVideoContainers()
{
    QStringList containers;

    QList<TInt> controllers = m_videoControllerMap.keys();
    for (int i = 0; i < controllers.count(); ++i) {
        foreach (VideoFormatData formatData, m_videoControllerMap[controllers[i]]) {
            for (int j = 0; j < formatData.supportedMimeTypes.count(); ++j) {
                if (containers.contains(formatData.supportedMimeTypes[j], Qt::CaseInsensitive) == false)
                    containers.append(formatData.supportedMimeTypes[j]);
            }
        }
    }

    return containers;
}

bool S60VideoCaptureSession::isSupportedVideoContainer(const QString &containerName)
{
    return supportedVideoContainers().contains(containerName, Qt::CaseInsensitive);
}

QString S60VideoCaptureSession::videoContainer() const
{
    return m_container;
}

void S60VideoCaptureSession::setVideoContainer(const QString &containerName)
{
    if (containerName == m_requestedContainer)
        return;

    if (containerName.isEmpty()) {
        m_requestedContainer = KMimeTypeDefaultContainer; // Use default
    } else {
        if (supportedVideoContainers().contains(containerName)) {
            m_requestedContainer = containerName;
        } else {
            setError(KErrNotSupported, tr("Requested video container is not supported."));
            m_requestedContainer = KMimeTypeDefaultContainer; // Reset to default
        }
    }

    m_uncommittedSettings = true;
}

QString S60VideoCaptureSession::videoContainerDescription(const QString &containerName)
{
    QList<TInt> formats;
    QList<TInt> encoders = m_videoControllerMap.keys();
    for (int i = 0; i < encoders.count(); ++i) {
        formats = m_videoControllerMap[encoders[i]].keys();
        for (int j = 0; j < formats.count(); ++j) {
            if (m_videoControllerMap[encoders[i]][formats[j]].supportedMimeTypes.contains(containerName, Qt::CaseInsensitive))
                return m_videoControllerMap[encoders[i]][formats[j]].description;
        }
    }

    return QString();
}

void S60VideoCaptureSession::cameraStatusChanged(QCamera::Status status)
{
    if (status == QCamera::ActiveStatus) {
        m_cameraStarted = true;

        // Continue preparation or start recording if previously requested
        if (m_captureState == EInitialized
            && (m_openWhenReady || m_prepareAfterOpenComplete || m_startAfterPrepareComplete)) {
            setOutputLocation(m_requestedSink);
            m_openWhenReady = false; // Reset
        } else if ((m_captureState == EOpenComplete || m_captureState == EPrepared)
            && (m_prepareAfterOpenComplete || m_startAfterPrepareComplete)) {
            startRecording();
            m_prepareAfterOpenComplete = false; // Reset
        }

    } else if (status == QCamera::UnloadedStatus) {
        m_cameraStarted = false;
        releaseVideoRecording();
    } else {
        m_cameraStarted = false;
    }
}

void S60VideoCaptureSession::durationTimerTriggered()
{
    // Update position only if recording is ongoing
    if ((m_captureState == ERecording) && m_videoRecorder) {
        // Signal will be automatically emitted of position changes
        TRAPD(err, m_position = m_videoRecorder->DurationL().Int64() / 1000);
        setError(err, tr("Cannot retrieve video position."));

        emit positionChanged(m_position);
    }
}

void S60VideoCaptureSession::doPopulateAudioCodecsL()
{
    if (m_captureState == EInitializing) {
        m_audioCodecList.clear();

        RArray<TFourCC> audioTypes;
        CleanupClosePushL(audioTypes);

        if (m_videoRecorder)
            m_videoRecorder->GetSupportedAudioTypesL(audioTypes);
        else
            setError(KErrNotReady, tr("Unexpected camera error."));

        for (TInt i = 0; i < audioTypes.Count(); i++) {
            TUint32 codec = audioTypes[i].FourCC();

            if (codec == KMMFFourCCCodeAMR)
                m_audioCodecList.insert(QString("audio/amr"), KMMFFourCCCodeAMR);
            if (codec == KMMFFourCCCodeAAC)
                m_audioCodecList.insert(QString("audio/aac"), KMMFFourCCCodeAAC);
        }
        CleanupStack::PopAndDestroy(&audioTypes);
    }
}

void S60VideoCaptureSession::doPopulateVideoCodecsL()
{
    if (m_captureState == EInitializing) {
        m_videoCodecList.clear();

        CDesC8ArrayFlat* videoTypes = new (ELeave) CDesC8ArrayFlat(10);
        CleanupStack::PushL(videoTypes);

        if (m_videoRecorder)
            m_videoRecorder->GetSupportedVideoTypesL(*videoTypes);
        else
            setError(KErrNotReady, tr("Unexpected camera error."));

        for (TInt i = 0; i < videoTypes->Count(); i++) {
            TPtrC8 videoType = videoTypes->MdcaPoint(i);
            QString codecMimeType = QString::fromUtf8((char *)videoType.Ptr(), videoType.Length());
#ifdef S60_DEVVIDEO_RECORDING_SUPPORTED
            for (int j = 0; j < m_videoParametersForEncoder.size(); ++j) {
                if (m_videoParametersForEncoder[j].mimeTypes.contains(codecMimeType, Qt::CaseInsensitive)) {
                    m_videoCodecList << codecMimeType;
                    break;
                }
            }
#else // CVideoRecorderUtility
            m_videoCodecList << codecMimeType;
#endif // S60_DEVVIDEO_RECORDING_SUPPORTED
        }
        CleanupStack::PopAndDestroy(videoTypes);
    }
}

#ifndef S60_DEVVIDEO_RECORDING_SUPPORTED
/*
 * Maximum resolution, framerate and bitrate can not be queried via MMF or
 * ECam, but needs to be set according to the definitions of the video
 * standard in question. In video standards, the values often depend on each
 * other, but the below function defines constant maximums.
 */
void S60VideoCaptureSession::doPopulateMaxVideoParameters()
{
    m_videoParametersForEncoder.append(MaxResolutionRatesAndTypes()); // For H.263
    m_videoParametersForEncoder.append(MaxResolutionRatesAndTypes()); // For MPEG-4
    m_videoParametersForEncoder.append(MaxResolutionRatesAndTypes()); // For H.264

    for (int i = 0; i < m_videoCodecList.count(); ++i) {

        // Use all lower case for comparisons
        QString codec = m_videoCodecList[i].toLower();

        if (codec.contains("video/h263-2000", Qt::CaseInsensitive)) {
            // H.263
            if (codec == "video/h263-2000" ||
                codec == "video/h263-2000; profile=0" ||
                codec == "video/h263-2000; profile=0; level=10" ||
                codec == "video/h263-2000; profile=3") {
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(15.0, QSize(176,144)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 64000)
                    m_videoParametersForEncoder[0].bitRate = 64000;
                continue;
            } else if (codec == "video/h263-2000; profile=0; level=20") {
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(15.0, QSize(352,288)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 128000)
                    m_videoParametersForEncoder[0].bitRate = 128000;
                continue;
            } else if (codec == "video/h263-2000; profile=0; level=30") {
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(30.0, QSize(352,288)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 384000)
                    m_videoParametersForEncoder[0].bitRate = 384000;
                continue;
            } else if (codec == "video/h263-2000; profile=0; level=40") {
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(30.0, QSize(352,288)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 2048000)
                    m_videoParametersForEncoder[0].bitRate = 2048000;
                continue;
            } else if (codec == "video/h263-2000; profile=0; level=45") {
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(15.0, QSize(176,144)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 128000)
                    m_videoParametersForEncoder[0].bitRate = 128000;
                continue;
            } else if (codec == "video/h263-2000; profile=0; level=50") {
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(15.0, QSize(352,288)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 4096000)
                    m_videoParametersForEncoder[0].bitRate = 4096000;
                continue;
            }

        } else if (codec.contains("video/mp4v-es", Qt::CaseInsensitive)) {
            // Mpeg-4
            if (codec == "video/mp4v-es" ||
                codec == "video/mp4v-es; profile-level-id=1" ||
                codec == "video/mp4v-es; profile-level-id=8") {
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(15.0, QSize(176,144)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 64000)
                    m_videoParametersForEncoder[0].bitRate = 64000;
                continue;
            } else if (codec == "video/mp4v-es; profile-level-id=2" ||
                       codec == "video/mp4v-es; profile-level-id=9") {
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(15.0, QSize(352,288)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 128000)
                    m_videoParametersForEncoder[0].bitRate = 128000;
                continue;
            } else if (codec == "video/mp4v-es; profile-level-id=3") {
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(30.0, QSize(352,288)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 384000)
                    m_videoParametersForEncoder[0].bitRate = 384000;
                continue;
            } else if (codec == "video/mp4v-es; profile-level-id=4") {
#if (defined(S60_31_PLATFORM) | defined(S60_32_PLATFORM))
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(15.0, QSize(640,480)));
#else // S60 5.0 and later platforms
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(30.0, QSize(640,480)));
#endif
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 4000000)
                    m_videoParametersForEncoder[0].bitRate = 4000000;
                continue;
            } else if (codec == "video/mp4v-es; profile-level-id=5") {
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(25.0, QSize(720,576)));
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(30.0, QSize(720,480)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 8000000)
                    m_videoParametersForEncoder[0].bitRate = 8000000;
                continue;
            } else if (codec == "video/mp4v-es; profile-level-id=6") {
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(30.0, QSize(1280,720)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 12000000)
                    m_videoParametersForEncoder[0].bitRate = 12000000;
                continue;
            }

        } else if (codec.contains("video/h264", Qt::CaseInsensitive)) {
            // H.264
            if (codec == "video/h264" ||
                codec == "video/h264; profile-level-id=42800a") {
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(15.0, QSize(176,144)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 64000)
                    m_videoParametersForEncoder[0].bitRate = 64000;
                continue;
            } else if (codec == "video/h264; profile-level-id=42900b") { // BP, L1b
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(15.0, QSize(176,144)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 128000)
                    m_videoParametersForEncoder[0].bitRate = 128000;
                continue;
            } else if (codec == "video/h264; profile-level-id=42800b") { // BP, L1.1
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(7.5, QSize(352,288)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 192000)
                    m_videoParametersForEncoder[0].bitRate = 192000;
                continue;
            } else if (codec == "video/h264; profile-level-id=42800c") { // BP, L1.2
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(15.0, QSize(352,288)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 384000)
                    m_videoParametersForEncoder[0].bitRate = 384000;
                continue;
            } else if (codec == "video/h264; profile-level-id=42800d") { // BP, L1.3
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(30.0, QSize(352,288)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 768000)
                    m_videoParametersForEncoder[0].bitRate = 768000;
                continue;
            } else if (codec == "video/h264; profile-level-id=428014") { // BP, L2
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(30.0, QSize(352,288)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 2000000)
                    m_videoParametersForEncoder[0].bitRate = 2000000;
                continue;
            } else if (codec == "video/h264; profile-level-id=428015") { // BP, L2.1
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(50.0, QSize(352,288)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 4000000)
                    m_videoParametersForEncoder[0].bitRate = 4000000;
                continue;
            } else if (codec == "video/h264; profile-level-id=428016") { // BP, L2.2
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(16.9, QSize(640,480)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 4000000)
                    m_videoParametersForEncoder[0].bitRate = 4000000;
                continue;
            } else if (codec == "video/h264; profile-level-id=42801e") { // BP, L3
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(33.8, QSize(640,480)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 10000000)
                    m_videoParametersForEncoder[0].bitRate = 10000000;
                continue;
            } else if (codec == "video/h264; profile-level-id=42801f") { // BP, L3.1
                m_videoParametersForEncoder[0].frameRatePictureSizePair.append(SupportedFrameRatePictureSize(30.0, QSize(1280,720)));
                m_videoParametersForEncoder[0].mimeTypes.append(codec);
                if (m_videoParametersForEncoder[0].bitRate < 14000000)
                    m_videoParametersForEncoder[0].bitRate = 14000000;
                continue;
            }
        }
    }
}
#endif // S60_DEVVIDEO_RECORDING_SUPPORTED

/*
 * This function returns the maximum resolution defined by the video standards
 * for different MIME Types.
 */
QSize S60VideoCaptureSession::maximumResolutionForMimeType(const QString &mimeType) const
{
    QSize maxSize(-1,-1);
    // Use all lower case for comparisons
    QString lowerMimeType = mimeType.toLower();

    if (lowerMimeType == "video/h263-2000") {
        maxSize = KResH263;
    } else if (lowerMimeType == "video/h263-2000; profile=0") {
        maxSize = KResH263_Profile0;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=10") {
        maxSize = KResH263_Profile0_Level10;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=20") {
        maxSize = KResH263_Profile0_Level20;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=30") {
        maxSize = KResH263_Profile0_Level30;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=40") {
        maxSize = KResH263_Profile0_Level40;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=45") {
        maxSize = KResH263_Profile0_Level45;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=50") {
        maxSize = KResH263_Profile0_Level50;
    } else if (lowerMimeType == "video/h263-2000; profile=3") {
        maxSize = KResH263_Profile3;
    } else if (lowerMimeType == "video/mp4v-es") {
        maxSize = KResMPEG4;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=1") {
        maxSize = KResMPEG4_PLID_1;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=2") {
        maxSize = KResMPEG4_PLID_2;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=3") {
        maxSize = KResMPEG4_PLID_3;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=4") {
        maxSize = KResMPEG4_PLID_4;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=5") {
        maxSize = KResMPEG4_PLID_5;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=6") {
        maxSize = KResMPEG4_PLID_6;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=8") {
        maxSize = KResMPEG4_PLID_8;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=9") {
        maxSize = KResMPEG4_PLID_9;
    } else if (lowerMimeType == "video/h264") {
        maxSize = KResH264;
    } else if (lowerMimeType == "video/h264; profile-level-id=42800a" ||
               lowerMimeType == "video/h264; profile-level-id=4d400a" ||
               lowerMimeType == "video/h264; profile-level-id=64400a") { // L1
        maxSize = KResH264_PLID_42800A;
    } else if (lowerMimeType == "video/h264; profile-level-id=42900b" ||
               lowerMimeType == "video/h264; profile-level-id=4d500b" ||
               lowerMimeType == "video/h264; profile-level-id=644009") { // L1.b
        maxSize = KResH264_PLID_42900B;
    } else if (lowerMimeType == "video/h264; profile-level-id=42800b" ||
               lowerMimeType == "video/h264; profile-level-id=4d400b" ||
               lowerMimeType == "video/h264; profile-level-id=64400b") { // L1.1
        maxSize = KResH264_PLID_42800B;
    } else if (lowerMimeType == "video/h264; profile-level-id=42800c" ||
               lowerMimeType == "video/h264; profile-level-id=4d400c" ||
               lowerMimeType == "video/h264; profile-level-id=64400c") { // L1.2
        maxSize = KResH264_PLID_42800C;
    } else if (lowerMimeType == "video/h264; profile-level-id=42800d" ||
               lowerMimeType == "video/h264; profile-level-id=4d400d" ||
               lowerMimeType == "video/h264; profile-level-id=64400d") { // L1.3
        maxSize = KResH264_PLID_42800D;
    } else if (lowerMimeType == "video/h264; profile-level-id=428014" ||
               lowerMimeType == "video/h264; profile-level-id=4d4014" ||
               lowerMimeType == "video/h264; profile-level-id=644014") { // L2
        maxSize = KResH264_PLID_428014;
    } else if (lowerMimeType == "video/h264; profile-level-id=428015" ||
               lowerMimeType == "video/h264; profile-level-id=4d4015" ||
               lowerMimeType == "video/h264; profile-level-id=644015") { // L2.1
        maxSize = KResH264_PLID_428015;
    } else if (lowerMimeType == "video/h264; profile-level-id=428016" ||
               lowerMimeType == "video/h264; profile-level-id=4d4016" ||
               lowerMimeType == "video/h264; profile-level-id=644016") { // L2.2
        maxSize = KResH264_PLID_428016;
    } else if (lowerMimeType == "video/h264; profile-level-id=42801e" ||
               lowerMimeType == "video/h264; profile-level-id=4d401e" ||
               lowerMimeType == "video/h264; profile-level-id=64401e") { // L3
        maxSize = KResH264_PLID_42801E;
    } else if (lowerMimeType == "video/h264; profile-level-id=42801f" ||
               lowerMimeType == "video/h264; profile-level-id=4d401f" ||
               lowerMimeType == "video/h264; profile-level-id=64401f") { // L3.1
        maxSize = KResH264_PLID_42801F;
    } else if (lowerMimeType == "video/h264; profile-level-id=428020" ||
               lowerMimeType == "video/h264; profile-level-id=4d4020" ||
               lowerMimeType == "video/h264; profile-level-id=644020") { // L3.2
        maxSize = KResH264_PLID_428020;
    } else if (lowerMimeType == "video/h264; profile-level-id=428028" ||
               lowerMimeType == "video/h264; profile-level-id=4d4028" ||
               lowerMimeType == "video/h264; profile-level-id=644028") { // L4
        maxSize = KResH264_PLID_428028;
    }

    return maxSize;
}

/*
 * This function returns the maximum framerate defined by the video standards
 * for different MIME Types.
 */

qreal S60VideoCaptureSession::maximumFrameRateForMimeType(const QString &mimeType) const
{
    qreal maxRate(-1.0);
    // Use all lower case for comparisons
    QString lowerMimeType = mimeType.toLower();

    if (lowerMimeType == "video/h263-2000") {
        maxRate = KFrR_H263;
    } else if (lowerMimeType == "video/h263-2000; profile=0") {
        maxRate = KFrR_H263_Profile0;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=10") {
        maxRate = KFrR_H263_Profile0_Level10;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=20") {
        maxRate = KFrR_H263_Profile0_Level20;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=30") {
        maxRate = KFrR_H263_Profile0_Level30;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=40") {
        maxRate = KFrR_H263_Profile0_Level40;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=45") {
        maxRate = KFrR_H263_Profile0_Level45;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=50") {
        maxRate = KFrR_H263_Profile0_Level50;
    } else if (lowerMimeType == "video/h263-2000; profile=3") {
        maxRate = KFrR_H263_Profile3;
    } else if (lowerMimeType == "video/mp4v-es") {
        maxRate = KFrR_MPEG4;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=1") {
        maxRate = KFrR_MPEG4_PLID_1;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=2") {
        maxRate = KFrR_MPEG4_PLID_2;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=3") {
        maxRate = KFrR_MPEG4_PLID_3;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=4") {
        maxRate = KFrR_MPEG4_PLID_4;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=5") {
        maxRate = KFrR_MPEG4_PLID_5;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=6") {
        maxRate = KFrR_MPEG4_PLID_6;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=8") {
        maxRate = KFrR_MPEG4_PLID_8;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=9") {
        maxRate = KFrR_MPEG4_PLID_9;
    } else if (lowerMimeType == "video/h264") {
        maxRate = KFrR_H264;
    } else if (lowerMimeType == "video/h264; profile-level-id=42800a" ||
               lowerMimeType == "video/h264; profile-level-id=4d400a" ||
               lowerMimeType == "video/h264; profile-level-id=64400a") { // L1
        maxRate = KFrR_H264_PLID_42800A;
    } else if (lowerMimeType == "video/h264; profile-level-id=42900b" ||
               lowerMimeType == "video/h264; profile-level-id=4d500b" ||
               lowerMimeType == "video/h264; profile-level-id=644009") { // L1.b
        maxRate = KFrR_H264_PLID_42900B;
    } else if (lowerMimeType == "video/h264; profile-level-id=42800b" ||
               lowerMimeType == "video/h264; profile-level-id=4d400b" ||
               lowerMimeType == "video/h264; profile-level-id=64400b") { // L1.1
        maxRate = KFrR_H264_PLID_42800B;
    } else if (lowerMimeType == "video/h264; profile-level-id=42800c" ||
               lowerMimeType == "video/h264; profile-level-id=4d400c" ||
               lowerMimeType == "video/h264; profile-level-id=64400c") { // L1.2
        maxRate = KFrR_H264_PLID_42800C;
    } else if (lowerMimeType == "video/h264; profile-level-id=42800d" ||
               lowerMimeType == "video/h264; profile-level-id=4d400d" ||
               lowerMimeType == "video/h264; profile-level-id=64400d") { // L1.3
        maxRate = KFrR_H264_PLID_42800D;
    } else if (lowerMimeType == "video/h264; profile-level-id=428014" ||
               lowerMimeType == "video/h264; profile-level-id=4d4014" ||
               lowerMimeType == "video/h264; profile-level-id=644014") { // L2
        maxRate = KFrR_H264_PLID_428014;
    } else if (lowerMimeType == "video/h264; profile-level-id=428015" ||
               lowerMimeType == "video/h264; profile-level-id=4d4015" ||
               lowerMimeType == "video/h264; profile-level-id=644015") { // L2.1
        maxRate = KFrR_H264_PLID_428015;
    } else if (lowerMimeType == "video/h264; profile-level-id=428016" ||
               lowerMimeType == "video/h264; profile-level-id=4d4016" ||
               lowerMimeType == "video/h264; profile-level-id=644016") { // L2.2
        maxRate = KFrR_H264_PLID_428016;
    } else if (lowerMimeType == "video/h264; profile-level-id=42801e" ||
               lowerMimeType == "video/h264; profile-level-id=4d401e" ||
               lowerMimeType == "video/h264; profile-level-id=64401e") { // L3
        maxRate = KFrR_H264_PLID_42801E;
    } else if (lowerMimeType == "video/h264; profile-level-id=42801f" ||
               lowerMimeType == "video/h264; profile-level-id=4d401f" ||
               lowerMimeType == "video/h264; profile-level-id=64401f") { // L3.1
        maxRate = KFrR_H264_PLID_42801F;
    } else if (lowerMimeType == "video/h264; profile-level-id=428020" ||
               lowerMimeType == "video/h264; profile-level-id=4d4020" ||
               lowerMimeType == "video/h264; profile-level-id=644020") { // L3.2
        maxRate = KFrR_H264_PLID_428020;
    } else if (lowerMimeType == "video/h264; profile-level-id=428028" ||
               lowerMimeType == "video/h264; profile-level-id=4d4028" ||
               lowerMimeType == "video/h264; profile-level-id=644028") { // L4
        maxRate = KFrR_H264_PLID_428028;
    }

    return maxRate;
}

/*
 * This function returns the maximum bitrate defined by the video standards
 * for different MIME Types.
 */
int S60VideoCaptureSession::maximumBitRateForMimeType(const QString &mimeType) const
{
    int maxRate(-1.0);
    // Use all lower case for comparisons
    QString lowerMimeType = mimeType.toLower();

    if (lowerMimeType == "video/h263-2000") {
        maxRate = KBiR_H263;
    } else if (lowerMimeType == "video/h263-2000; profile=0") {
        maxRate = KBiR_H263_Profile0;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=10") {
        maxRate = KBiR_H263_Profile0_Level10;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=20") {
        maxRate = KBiR_H263_Profile0_Level20;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=30") {
        maxRate = KBiR_H263_Profile0_Level30;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=40") {
        maxRate = KBiR_H263_Profile0_Level40;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=45") {
        maxRate = KBiR_H263_Profile0_Level45;
    } else if (lowerMimeType == "video/h263-2000; profile=0; level=50") {
        maxRate = KBiR_H263_Profile0_Level50;
    } else if (lowerMimeType == "video/h263-2000; profile=3") {
        maxRate = KBiR_H263_Profile3;
    } else if (lowerMimeType == "video/mp4v-es") {
        maxRate = KBiR_MPEG4;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=1") {
        maxRate = KBiR_MPEG4_PLID_1;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=2") {
        maxRate = KBiR_MPEG4_PLID_2;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=3") {
        maxRate = KBiR_MPEG4_PLID_3;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=4") {
        maxRate = KBiR_MPEG4_PLID_4;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=5") {
        maxRate = KBiR_MPEG4_PLID_5;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=6") {
        maxRate = KBiR_MPEG4_PLID_6;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=8") {
        maxRate = KBiR_MPEG4_PLID_8;
    } else if (lowerMimeType == "video/mp4v-es; profile-level-id=9") {
        maxRate = KBiR_MPEG4_PLID_9;
    } else if (lowerMimeType == "video/h264") {
        maxRate = KBiR_H264;
    } else if (lowerMimeType == "video/h264; profile-level-id=42800a" ||
               lowerMimeType == "video/h264; profile-level-id=4d400a" ||
               lowerMimeType == "video/h264; profile-level-id=64400a") { // L1
        maxRate = KBiR_H264_PLID_42800A;
    } else if (lowerMimeType == "video/h264; profile-level-id=42900b" ||
               lowerMimeType == "video/h264; profile-level-id=4d500b" ||
               lowerMimeType == "video/h264; profile-level-id=644009") { // L1.b
        maxRate = KBiR_H264_PLID_42900B;
    } else if (lowerMimeType == "video/h264; profile-level-id=42800b" ||
               lowerMimeType == "video/h264; profile-level-id=4d400b" ||
               lowerMimeType == "video/h264; profile-level-id=64400b") { // L1.1
        maxRate = KBiR_H264_PLID_42800B;
    } else if (lowerMimeType == "video/h264; profile-level-id=42800c" ||
               lowerMimeType == "video/h264; profile-level-id=4d400c" ||
               lowerMimeType == "video/h264; profile-level-id=64400c") { // L1.2
        maxRate = KBiR_H264_PLID_42800C;
    } else if (lowerMimeType == "video/h264; profile-level-id=42800d" ||
               lowerMimeType == "video/h264; profile-level-id=4d400d" ||
               lowerMimeType == "video/h264; profile-level-id=64400d") { // L1.3
        maxRate = KBiR_H264_PLID_42800D;
    } else if (lowerMimeType == "video/h264; profile-level-id=428014" ||
               lowerMimeType == "video/h264; profile-level-id=4d4014" ||
               lowerMimeType == "video/h264; profile-level-id=644014") { // L2
        maxRate = KBiR_H264_PLID_428014;
    } else if (lowerMimeType == "video/h264; profile-level-id=428015" ||
               lowerMimeType == "video/h264; profile-level-id=4d4015" ||
               lowerMimeType == "video/h264; profile-level-id=644015") { // L2.1
        maxRate = KBiR_H264_PLID_428015;
    } else if (lowerMimeType == "video/h264; profile-level-id=428016" ||
               lowerMimeType == "video/h264; profile-level-id=4d4016" ||
               lowerMimeType == "video/h264; profile-level-id=644016") { // L2.2
        maxRate = KBiR_H264_PLID_428016;
    } else if (lowerMimeType == "video/h264; profile-level-id=42801e" ||
               lowerMimeType == "video/h264; profile-level-id=4d401e" ||
               lowerMimeType == "video/h264; profile-level-id=64401e") { // L3
        maxRate = KBiR_H264_PLID_42801E;
    } else if (lowerMimeType == "video/h264; profile-level-id=42801f" ||
               lowerMimeType == "video/h264; profile-level-id=4d401f" ||
               lowerMimeType == "video/h264; profile-level-id=64401f") { // L3.1
        maxRate = KBiR_H264_PLID_42801F;
    } else if (lowerMimeType == "video/h264; profile-level-id=428020" ||
               lowerMimeType == "video/h264; profile-level-id=4d4020" ||
               lowerMimeType == "video/h264; profile-level-id=644020") { // L3.2
        maxRate = KBiR_H264_PLID_428020;
    } else if (lowerMimeType == "video/h264; profile-level-id=428028" ||
               lowerMimeType == "video/h264; profile-level-id=4d4028" ||
               lowerMimeType == "video/h264; profile-level-id=644028") { // L4
        maxRate = KBiR_H264_PLID_428028;
    }

    return maxRate;
}

// End of file
