/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "avfcameradebug_p.h"
#include "avfmediarecordercontrol_p.h"
#include "avfcamerasession_p.h"
#include "avfcameraservice_p.h"
#include "avfcameracontrol_p.h"

#include "qaudiodeviceinfo.h"
#include "qmediadevicemanager.h"
#include "private/qdarwinformatsinfo_p.h"
#include "avfcamerautility_p.h"

#include <QtCore/qurl.h>
#include <QtCore/qmimetype.h>
#include <QtCore/qfileinfo.h>
#include <private/qplatformcamera_p.h>
#include <CoreAudio/CoreAudio.h>

QT_USE_NAMESPACE

@interface AVFMediaRecorderDelegate : NSObject <AVCaptureFileOutputRecordingDelegate>
{
}

- (AVFMediaRecorderDelegate *) initWithRecorder:(AVFMediaRecorderControl*)recorder;

- (void) captureOutput:(AVCaptureFileOutput *)captureOutput
         didStartRecordingToOutputFileAtURL:(NSURL *)fileURL
         fromConnections:(NSArray *)connections;

- (void) captureOutput:(AVCaptureFileOutput *)captureOutput
         didFinishRecordingToOutputFileAtURL:(NSURL *)fileURL
         fromConnections:(NSArray *)connections
         error:(NSError *)error;
@end

@implementation AVFMediaRecorderDelegate
{
    @private
        AVFMediaRecorderControl *m_recorder;
}

- (AVFMediaRecorderDelegate *) initWithRecorder:(AVFMediaRecorderControl*)recorder
{
    if (!(self = [super init]))
        return nil;

    self->m_recorder = recorder;
    return self;
}

- (void) captureOutput:(AVCaptureFileOutput *)captureOutput
         didStartRecordingToOutputFileAtURL:(NSURL *)fileURL
         fromConnections:(NSArray *)connections
{
    Q_UNUSED(captureOutput);
    Q_UNUSED(fileURL);
    Q_UNUSED(connections);

    QMetaObject::invokeMethod(m_recorder, "handleRecordingStarted", Qt::QueuedConnection);
}

- (void) captureOutput:(AVCaptureFileOutput *)captureOutput
         didFinishRecordingToOutputFileAtURL:(NSURL *)fileURL
         fromConnections:(NSArray *)connections
         error:(NSError *)error
{
    Q_UNUSED(captureOutput);
    Q_UNUSED(fileURL);
    Q_UNUSED(connections);

    if (error) {
        QStringList messageParts;
        messageParts << QString::fromUtf8([[error localizedDescription] UTF8String]);
        messageParts << QString::fromUtf8([[error localizedFailureReason] UTF8String]);
        messageParts << QString::fromUtf8([[error localizedRecoverySuggestion] UTF8String]);

        QString errorMessage = messageParts.join(QChar(u' '));

        QMetaObject::invokeMethod(m_recorder, "handleRecordingFailed", Qt::QueuedConnection,
                                  Q_ARG(QString, errorMessage));
    } else {
        QMetaObject::invokeMethod(m_recorder, "handleRecordingFinished", Qt::QueuedConnection);
    }
}

@end


AVFMediaRecorderControl::AVFMediaRecorderControl(AVFCameraService *service, QObject *parent)
   : QPlatformMediaRecorder(parent)
   , m_cameraControl(service->avfCameraControl())
   , m_session(service->session())
   , m_connected(false)
   , m_state(QMediaRecorder::StoppedState)
   , m_lastStatus(QMediaRecorder::UnloadedStatus)
   , m_recordingStarted(false)
   , m_recordingFinished(false)
   , m_muted(false)
   , m_volume(1.0)
   , m_audioInput(nil)
   , m_restoreFPS(-1, -1)
{
    m_movieOutput = [[AVCaptureMovieFileOutput alloc] init];
    m_recorderDelagate = [[AVFMediaRecorderDelegate alloc] initWithRecorder:this];

    m_audioCaptureDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];

    connect(m_cameraControl, SIGNAL(stateChanged(QCamera::State)), SLOT(updateStatus()));
    connect(m_cameraControl, SIGNAL(statusChanged(QCamera::Status)), SLOT(updateStatus()));
    connect(m_session, SIGNAL(readyToConfigureConnections()), SLOT(setupSessionForCapture()));
    connect(m_session, SIGNAL(stateChanged(QCamera::State)), SLOT(setupSessionForCapture()));
}

AVFMediaRecorderControl::~AVFMediaRecorderControl()
{
    if (m_movieOutput) {
        [m_session->captureSession() removeOutput:m_movieOutput];
        [m_movieOutput release];
    }

    if (m_audioInput) {
        [m_session->captureSession() removeInput:m_audioInput];
        [m_audioInput release];
    }

    [m_recorderDelagate release];
}

QUrl AVFMediaRecorderControl::outputLocation() const
{
    return m_outputLocation;
}

bool AVFMediaRecorderControl::setOutputLocation(const QUrl &location)
{
    m_outputLocation = location;
    return location.scheme() == QLatin1String("file") || location.scheme().isEmpty();
}

QMediaRecorder::State AVFMediaRecorderControl::state() const
{
    return m_state;
}

QMediaRecorder::Status AVFMediaRecorderControl::status() const
{
    QMediaRecorder::Status status = m_lastStatus;
    //bool videoEnabled = m_cameraControl->captureMode().testFlag(QCamera::CaptureVideo);

    if (m_cameraControl->status() == QCamera::ActiveStatus && m_connected) {
        if (m_state == QMediaRecorder::StoppedState) {
            if (m_recordingStarted && !m_recordingFinished)
                status = QMediaRecorder::FinalizingStatus;
            else
                status = QMediaRecorder::LoadedStatus;
        } else {
            status = m_recordingStarted ? QMediaRecorder::RecordingStatus :
                                            QMediaRecorder::StartingStatus;
        }
    } else {
        //camera not started yet
        status = m_cameraControl->state() == QCamera::ActiveState && m_connected ?
                    QMediaRecorder::LoadingStatus:
                    QMediaRecorder::UnloadedStatus;
    }

    return status;
}

void AVFMediaRecorderControl::updateStatus()
{
    QMediaRecorder::Status newStatus = status();

    if (m_lastStatus != newStatus) {
        qDebugCamera() << "Camera recorder status changed: " << m_lastStatus << " -> " << newStatus;
        m_lastStatus = newStatus;
        Q_EMIT statusChanged(m_lastStatus);
    }
}


qint64 AVFMediaRecorderControl::duration() const
{
    if (!m_movieOutput)
        return 0;

    return qint64(CMTimeGetSeconds(m_movieOutput.recordedDuration) * 1000);
}

bool AVFMediaRecorderControl::isMuted() const
{
    return m_muted;
}

qreal AVFMediaRecorderControl::volume() const
{
    return m_volume;
}

static NSDictionary *avfAudioSettings(const QMediaEncoderSettings &encoderSettings)
{
    NSMutableDictionary *settings = [NSMutableDictionary dictionary];

    int codecId = QDarwinFormatInfo::audioFormatForCodec(encoderSettings.audioCodec());

    [settings setObject:[NSNumber numberWithInt:codecId] forKey:AVFormatIDKey];

#ifdef Q_OS_OSX
    if (encoderSettings.encodingMode() == QMultimedia::ConstantQualityEncoding) {
        int quality;
        switch (encoderSettings.quality()) {
        case QMultimedia::VeryLowQuality:
            quality = AVAudioQualityMin;
            break;
        case QMultimedia::LowQuality:
            quality = AVAudioQualityLow;
            break;
        case QMultimedia::HighQuality:
            quality = AVAudioQualityHigh;
            break;
        case QMultimedia::VeryHighQuality:
            quality = AVAudioQualityMax;
            break;
        case QMultimedia::NormalQuality:
        default:
            quality = AVAudioQualityMedium;
            break;
        }
        [settings setObject:[NSNumber numberWithInt:quality] forKey:AVEncoderAudioQualityKey];

    } else
#endif
    if (encoderSettings.audioBitRate() > 0)
        [settings setObject:[NSNumber numberWithInt:encoderSettings.audioBitRate()] forKey:AVEncoderBitRateKey];

    int sampleRate = encoderSettings.audioSampleRate();
    int channelCount = encoderSettings.audioChannelCount();

#ifdef Q_OS_IOS
    // Some keys are mandatory only on iOS
    if (sampleRate <= 0)
        sampleRate = 44100;
    if (channelCount <= 0)
        channelCount = 2;
#endif

    if (sampleRate > 0)
        [settings setObject:[NSNumber numberWithInt:sampleRate] forKey:AVSampleRateKey];
    if (channelCount > 0)
        [settings setObject:[NSNumber numberWithInt:channelCount] forKey:AVNumberOfChannelsKey];

    return settings;
}

static bool formatSupportsFramerate(AVCaptureDeviceFormat *format, qreal fps)
{
    if (format && fps > qreal(0)) {
        const qreal epsilon = 0.1;
        for (AVFrameRateRange *range in format.videoSupportedFrameRateRanges) {
            if (range.maxFrameRate - range.minFrameRate < epsilon) {
                if (qAbs(fps - range.maxFrameRate) < epsilon)
                    return true;
            }

            if (fps >= range.minFrameRate && fps <= range.maxFrameRate)
                return true;
        }
    }

    return false;
}

NSDictionary *avfVideoSettings(QMediaEncoderSettings &encoderSettings, AVCaptureDevice *device, AVCaptureConnection *connection)
{
    if (!device)
        return nil;

    // ### How to set file type????
    // Maybe need to use AVCaptureVideoDataOutput on macOS as well?

    // ### re-add needFpsChange
//    AVFPSRange currentFps = qt_current_framerates(device, connection);

    NSMutableDictionary *videoSettings = [NSMutableDictionary dictionary];

    // -- Codec

    // AVVideoCodecKey is the only mandatory key
    auto codec = encoderSettings.videoCodec();
    NSString *c = QDarwinFormatInfo::videoFormatForCodec(codec);
    [videoSettings setObject:c forKey:AVVideoCodecKey];
    [c release];

    // -- Resolution

    int w = encoderSettings.videoResolution().width();
    int h = encoderSettings.videoResolution().height();

    if (AVCaptureDeviceFormat *currentFormat = device.activeFormat) {
        CMFormatDescriptionRef formatDesc = currentFormat.formatDescription;
        CMVideoDimensions dim = CMVideoFormatDescriptionGetDimensions(formatDesc);
        FourCharCode formatCodec = CMVideoFormatDescriptionGetCodecType(formatDesc);

        // We have to change the device's activeFormat in 3 cases:
        // - the requested recording resolution is higher than the current device resolution
        // - the requested recording resolution has a different aspect ratio than the current device aspect ratio
        // - the requested frame rate is not available for the current device format
        AVCaptureDeviceFormat *newFormat = nil;
        if ((w <= 0 || h <= 0)
                && encoderSettings.videoFrameRate() > 0
                && !formatSupportsFramerate(currentFormat, encoderSettings.videoFrameRate())) {

            newFormat = qt_find_best_framerate_match(device,
                                                     formatCodec,
                                                     encoderSettings.videoFrameRate());

        } else if (w > 0 && h > 0) {
            AVCaptureDeviceFormat *f = qt_find_best_resolution_match(device,
                                                                     encoderSettings.videoResolution(),
                                                                     formatCodec);

            if (f) {
                CMVideoDimensions d = CMVideoFormatDescriptionGetDimensions(f.formatDescription);
                qreal fAspectRatio = qreal(d.width) / d.height;

                if (w > dim.width || h > dim.height
                        || qAbs((qreal(dim.width) / dim.height) - fAspectRatio) > 0.01) {
                    newFormat = f;
                }
            }
        }

        if (qt_set_active_format(device, newFormat, false /*### !needFpsChange*/)) {
            formatDesc = newFormat.formatDescription;
            dim = CMVideoFormatDescriptionGetDimensions(formatDesc);
        }

        if (w > 0 && h > 0) {
            // Make sure the recording resolution has the same aspect ratio as the device's
            // current resolution
            qreal deviceAspectRatio = qreal(dim.width) / dim.height;
            qreal recAspectRatio = qreal(w) / h;
            if (qAbs(deviceAspectRatio - recAspectRatio) > 0.01) {
                if (recAspectRatio > deviceAspectRatio)
                    w = qRound(h * deviceAspectRatio);
                else
                    h = qRound(w / deviceAspectRatio);
            }

            // recording resolution can't be higher than the device's active resolution
            w = qMin(w, dim.width);
            h = qMin(h, dim.height);
        }
    }

    if (w > 0 && h > 0) {
        // Width and height must be divisible by 2
        w += w & 1;
        h += h & 1;

        [videoSettings setObject:[NSNumber numberWithInt:w] forKey:AVVideoWidthKey];
        [videoSettings setObject:[NSNumber numberWithInt:h] forKey:AVVideoHeightKey];
        encoderSettings.setVideoResolution(w, h);
    } else {
        encoderSettings.setVideoResolution(qt_device_format_resolution(device.activeFormat));
    }

    // -- FPS

    if (true /*needFpsChange*/) {
        const qreal fps = encoderSettings.videoFrameRate();
        qt_set_framerate_limits(device, connection, fps, fps);
    }
    encoderSettings.setVideoFrameRate(qt_current_framerates(device, connection).second);

    // -- Codec Settings

    NSMutableDictionary *codecProperties = [NSMutableDictionary dictionary];
    int bitrate = -1;
    float quality = -1.f;

    if (encoderSettings.encodingMode() == QMultimedia::ConstantQualityEncoding) {
        if (encoderSettings.quality() != QMultimedia::NormalQuality) {
            if (codec != QMediaFormat::VideoCodec::MotionJPEG) {
                qWarning("ConstantQualityEncoding is not supported for MotionJPEG");
            } else {
                switch (encoderSettings.quality()) {
                case QMultimedia::VeryLowQuality:
                    quality = 0.f;
                    break;
                case QMultimedia::LowQuality:
                    quality = 0.25f;
                    break;
                case QMultimedia::HighQuality:
                    quality = 0.75f;
                    break;
                case QMultimedia::VeryHighQuality:
                    quality = 1.f;
                    break;
                default:
                    quality = -1.f; // NormalQuality, let the system decide
                    break;
                }
            }
        }
    } else if (encoderSettings.encodingMode() == QMultimedia::AverageBitRateEncoding){
        if (codec != QMediaFormat::VideoCodec::H264 && codec != QMediaFormat::VideoCodec::H265)
            qWarning() << "AverageBitRateEncoding is not supported for codec" <<  QMediaFormat::videoCodecName(codec);
        else
            bitrate = encoderSettings.videoBitRate();
    } else {
        qWarning("Encoding mode is not supported");
    }

    if (bitrate != -1)
        [codecProperties setObject:[NSNumber numberWithInt:bitrate] forKey:AVVideoAverageBitRateKey];
    if (quality != -1.f)
        [codecProperties setObject:[NSNumber numberWithFloat:quality] forKey:AVVideoQualityKey];

    [videoSettings setObject:codecProperties forKey:AVVideoCompressionPropertiesKey];

    return videoSettings;
}

void AVFMediaRecorderControl::applySettings()
{
    if (m_state != QMediaRecorder::StoppedState
            || (m_session->state() != QCamera::ActiveState && m_session->state() != QCamera::LoadedState))
        return;

    QMediaEncoderSettings resolved = m_settings;
    resolved.resolveFormat();
    qDebug() << "file  profile" << QMediaFormat::fileFormatName(resolved.format());
    qDebug() << "video profile" << QMediaFormat::videoCodecName(resolved.videoCodec());
    qDebug() << "audio profile" << QMediaFormat::audioCodecName(resolved.audioCodec());

    const AVFConfigurationLock lock(m_session->videoCaptureDevice()); // prevents activeFormat from being overridden

    // Configure audio settings
    [m_movieOutput setOutputSettings:avfAudioSettings(resolved)
                   forConnection:[m_movieOutput connectionWithMediaType:AVMediaTypeAudio]];

    // Configure video settings
    AVCaptureConnection *videoConnection = [m_movieOutput connectionWithMediaType:AVMediaTypeVideo];
    AVCaptureDevice *captureDevice = m_session->videoCaptureDevice();

    NSDictionary *videoSettings = avfVideoSettings(resolved, captureDevice, videoConnection);

    [m_movieOutput setOutputSettings:videoSettings forConnection:videoConnection];
}

void AVFMediaRecorderControl::unapplySettings()
{
//    m_service->audioEncoderSettingsControl()->unapplySettings();
//    m_service->videoEncoderSettingsControl()->unapplySettings([m_movieOutput connectionWithMediaType:AVMediaTypeVideo]);
}

QAudioDeviceInfo AVFMediaRecorderControl::audioInput() const
{
    QByteArray id = [[m_audioCaptureDevice uniqueID] UTF8String];
    const QList<QAudioDeviceInfo> devices = QMediaDeviceManager::audioInputs();
    for (auto d : devices)
        if (d.id() == id)
            return d;
    return QMediaDeviceManager::defaultAudioInput();
}

bool AVFMediaRecorderControl::setAudioInput(const QAudioDeviceInfo &id)
{
    AVCaptureDevice *device = nullptr;

    if (!id.isNull()) {
        device = [AVCaptureDevice deviceWithUniqueID: [NSString stringWithUTF8String:id.id().constData()]];
    } else {
        device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];
    }

    if (device) {
        m_audioCaptureDevice = device;
        return true;
    }
    return false;
}

void AVFMediaRecorderControl::setEncoderSettings(const QMediaEncoderSettings &settings)
{
    m_settings = settings;
}

void AVFMediaRecorderControl::setState(QMediaRecorder::State state)
{
    if (m_state == state)
        return;

    qDebugCamera() << Q_FUNC_INFO << m_state << " -> " << state;

    switch (state) {
    case QMediaRecorder::RecordingState:
    {
        if (m_connected) {
            QString outputLocationPath = m_outputLocation.scheme() == QLatin1String("file") ?
                        m_outputLocation.path() : m_outputLocation.toString();

            QMediaEncoderSettings resolved = m_settings;
            resolved.resolveFormat();
            QString extension = resolved.mimeType().preferredSuffix();

            QUrl actualLocation = QUrl::fromLocalFile(
                        m_storageLocation.generateFileName(outputLocationPath,
                                                           AVFStorageLocation::Video,
                                                           QLatin1String("clip_"),
                                                           extension));

            qDebugCamera() << "Video capture location:" << actualLocation.toString();

//            applySettings();

            [m_movieOutput startRecordingToOutputFileURL:actualLocation.toNSURL()
                           recordingDelegate:m_recorderDelagate];

            m_state = QMediaRecorder::RecordingState;
            m_recordingStarted = false;
            m_recordingFinished = false;

            Q_EMIT actualLocationChanged(actualLocation);
            updateStatus();
            Q_EMIT stateChanged(m_state);
        } else {
            Q_EMIT error(QMediaRecorder::FormatError, tr("Recorder not configured"));
        }

    } break;
    case QMediaRecorder::PausedState:
    {
        Q_EMIT error(QMediaRecorder::FormatError, tr("Recording pause not supported"));
        return;
    } break;
    case QMediaRecorder::StoppedState:
    {
        m_lastStatus = QMediaRecorder::FinalizingStatus;
        Q_EMIT statusChanged(m_lastStatus);
        [m_movieOutput stopRecording];
        unapplySettings();
    }
    }
}

void AVFMediaRecorderControl::setMuted(bool muted)
{
    if (m_muted != muted) {
        m_muted = muted;
        Q_EMIT mutedChanged(muted);
    }
}

void AVFMediaRecorderControl::setVolume(qreal volume)
{
    if (m_volume != volume) {
        m_volume = volume;
        Q_EMIT volumeChanged(volume);
    }
}

void AVFMediaRecorderControl::handleRecordingStarted()
{
    m_recordingStarted = true;
    updateStatus();
}

void AVFMediaRecorderControl::handleRecordingFinished()
{
    m_recordingFinished = true;
    if (m_state != QMediaRecorder::StoppedState) {
        m_state = QMediaRecorder::StoppedState;
        Q_EMIT stateChanged(m_state);
    }
    updateStatus();
}

void AVFMediaRecorderControl::handleRecordingFailed(const QString &message)
{
    m_recordingFinished = true;
    if (m_state != QMediaRecorder::StoppedState) {
        m_state = QMediaRecorder::StoppedState;
        Q_EMIT stateChanged(m_state);
    }
    updateStatus();

    Q_EMIT error(QMediaRecorder::ResourceError, message);
}

void AVFMediaRecorderControl::setupSessionForCapture()
{
    if (!m_session->videoCaptureDevice())
        return;

    //adding movie output causes high CPU usage even when while recording is not active,
    //connect it only while video capture mode is enabled.
    // Similarly, connect the Audio input only in that mode, since it's only necessary
    // when recording anyway. Adding an Audio input will trigger the microphone permission
    // request on iOS, but it shoudn't do so until we actually try to record.
    AVCaptureSession *captureSession = m_session->captureSession();

    if (!m_connected && m_session->state() != QCamera::UnloadedState) {

        // Lock the video capture device to make sure the active format is not reset
        const AVFConfigurationLock lock(m_session->videoCaptureDevice());

        // Add audio input
        // Allow recording even if something wrong happens with the audio input initialization
        AVCaptureDevice *audioDevice = m_audioCaptureDevice;
        if (!audioDevice) {
            qWarning("No audio input device available");
        } else {
            NSError *error = nil;
            m_audioInput = [AVCaptureDeviceInput deviceInputWithDevice:audioDevice error:&error];

            if (!m_audioInput) {
                qWarning() << "Failed to create audio device input";
            } else if (![captureSession canAddInput:m_audioInput]) {
                qWarning() << "Could not connect the audio input";
                m_audioInput = nullptr;
            } else {
                [m_audioInput retain];
                [captureSession addInput:m_audioInput];
            }
        }

        if ([captureSession canAddOutput:m_movieOutput]) {
            [captureSession addOutput:m_movieOutput];
            m_connected = true;
        } else {
            Q_EMIT error(QMediaRecorder::ResourceError, tr("Could not connect the video recorder"));
            qWarning() << "Could not connect the video recorder";
        }
    } else if (m_connected || m_session->state() == QCamera::UnloadedState) {

        // Lock the video capture device to make sure the active format is not reset
        const AVFConfigurationLock lock(m_session->videoCaptureDevice());

        [captureSession removeOutput:m_movieOutput];

        if (m_audioInput) {
            [captureSession removeInput:m_audioInput];
            [m_audioInput release];
            m_audioInput = nil;
        }

        m_connected = false;
    }

    updateStatus();
}

#include "moc_avfmediarecordercontrol_p.cpp"
