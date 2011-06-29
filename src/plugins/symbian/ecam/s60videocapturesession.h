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

#ifndef S60VIDEOCAPTURESESSION_H
#define S60VIDEOCAPTURESESSION_H

#include <QtCore/qurl.h>
#include <QtCore/qhash.h>

#include <qmediaencodersettings.h>
#include <qcamera.h>
#include <qmediarecorder.h>

#include "s60cameraengine.h"

#include <e32base.h>
#include <videorecorder.h> // CVideoRecorderUtility
#ifdef S60_DEVVIDEO_RECORDING_SUPPORTED
#include <mmf/devvideo/devvideorecord.h>
#endif // S60_DEVVIDEO_RECORDING_SUPPORTED

QT_USE_NAMESPACE

class QTimer;

/*
 * VideoSession is the main class handling all video recording related
 * operations. It uses mainly CVideoRecorderUtility to do it's tasks, but if
 * DevVideoRecord is available it is used to provide more detailed
 * information of the supported video settings.
 */
class S60VideoCaptureSession : public QObject,
                               public MVideoRecorderUtilityObserver
#ifdef S60_DEVVIDEO_RECORDING_SUPPORTED
                               ,public MMMFDevVideoRecordObserver
#endif // S60_DEVVIDEO_RECORDING_SUPPORTED
{
    Q_OBJECT
    Q_ENUMS(Error)
    Q_ENUMS(EcamErrors)
    Q_ENUMS(TVideoCaptureState)

public: // Enums

    enum TVideoCaptureState
    {
        ENotInitialized = 0,    // 0 - VideoRecording is not initialized, instance may or may not be created
        EInitializing,          // 1 - Initialization is ongoing
        EInitialized,           // 2 - VideoRecording is initialized, OpenFile is called with dummy file
        EOpening,               // 3 - OpenFile called with actual output location, waiting completion
        EOpenComplete,          // 4 - OpenFile completed with the actual output location
        EPreparing,             // 5 - Preparing VideoRecording to use set video settings
        EPrepared,              // 6 - VideoRecording is prepared with the set settings, ready to record
        ERecording,             // 7 - Video recording is ongoing
        EPaused                 // 8 - Video recording has been started and paused
    };

    enum AudioQualityDefinition
    {
        ENoAudioQuality = 0,        // 0 - Both BitRate and SampleRate settings available
        EOnlyAudioQuality,          // 1 - No BitRate or SampleRate settings available, use Quality to set them
        EAudioQualityAndBitRate,    // 2 - BitRate setting available, use Quality to set SampleRate
        EAudioQualityAndSampleRate, // 3 - SampleRate setting available, use Quality to set BitRate
    };

    enum VideoQualityDefinition
    {
        ENoVideoQuality = 0,        // 0 - All, Resolution, FrameRate and BitRate available
        EOnlyVideoQuality,          // 1 - None available, use Quality to set Resolution, FrameRate and BitRate
        EVideoQualityAndResolution, // 2 - Only Resolution available, use Quality to set FrameRate and BitRate
        EVideoQualityAndFrameRate,  // 3 - Only FrameRate available, use Quality to set Resolution and BitRate
        EVideoQualityAndBitRate,    // 4 - Only BitRate available, use Quality to set Resolution and FrameRate
        EVideoQualityAndResolutionAndBitRate,   // 5 - No FrameRate available, use Quality to set it
        EVideoQualityAndResolutionAndFrameRate, // 6 - No BitRate available, use Quality to set it
        EVideoQualityAndFrameRateAndBitRate     // 7 - No Resolution available, use Quality to set it
    };

public: // Constructor & Destructor

    S60VideoCaptureSession(QObject *parent = 0);
    ~S60VideoCaptureSession();

public: // MVideoRecorderUtilityObserver

    void MvruoOpenComplete(TInt aError);
    void MvruoPrepareComplete(TInt aError);
    void MvruoRecordComplete(TInt aError);
    void MvruoEvent(const TMMFEvent& aEvent);

#ifdef S60_DEVVIDEO_RECORDING_SUPPORTED
public: // MMMFDevVideoRecordObserver
    void MdvroReturnPicture(TVideoPicture *aPicture);
    void MdvroSupplementalInfoSent();
    void MdvroNewBuffers();
    void MdvroFatalError(TInt aError);
    void MdvroInitializeComplete(TInt aError);
    void MdvroStreamEnd();
#endif // S60_DEVVIDEO_RECORDING_SUPPORTED

public: // Methods

    void setError(const TInt error, const QString &description);
    void setCameraHandle(CCameraEngine* cameraHandle);
    void notifySettingsSet();

    qint64 position();
    TVideoCaptureState state() const;
    bool isMuted() const;

    // Controls
    int initializeVideoRecording();
    void releaseVideoRecording();
    void applyAllSettings();

    void startRecording();
    void pauseRecording();
    void stopRecording(const bool reInitialize = true);
    void setMuted(const bool muted);

    // Output Location
    bool setOutputLocation(const QUrl &sink);
    QUrl outputLocation() const;

    // Resolution
    void setVideoResolution(const QSize &resolution);
    QList<QSize> supportedVideoResolutions(bool *continuous);
    QList<QSize> supportedVideoResolutions(const QVideoEncoderSettings &settings, bool *continuous);

    // Framerate
    void setFrameRate(const qreal rate);
    QList<qreal> supportedVideoFrameRates(bool *continuous);
    QList<qreal> supportedVideoFrameRates(const QVideoEncoderSettings &settings, bool *continuous);

    // Other Video Settings
    void setBitrate(const int bitrate);
    void setVideoEncodingMode(const QtMultimediaKit::EncodingMode mode);

    // Video Codecs
    void setVideoCaptureCodec(const QString &codecName);
    QStringList supportedVideoCaptureCodecs();
    QString videoCaptureCodecDescription(const QString &codecName);

    // Audio Codecs
    void setAudioCaptureCodec(const QString &codecName);
    QStringList supportedAudioCaptureCodecs();

    // Encoder Settings
    void videoEncoderSettings(QVideoEncoderSettings &videoSettings);
    void audioEncoderSettings(QAudioEncoderSettings &audioSettings);

    // Quality
    void setVideoCaptureQuality(const QtMultimediaKit::EncodingQuality quality,
                                const VideoQualityDefinition mode);
    void setAudioCaptureQuality(const QtMultimediaKit::EncodingQuality quality,
                                const AudioQualityDefinition mode);

    // Video Containers
    QString videoContainer() const;
    void setVideoContainer(const QString &containerName);
    QStringList supportedVideoContainers();
    bool isSupportedVideoContainer(const QString &containerName);
    QString videoContainerDescription(const QString &containerName);

    // Audio Settings
    QList<int> supportedSampleRates(const QAudioEncoderSettings &settings, bool *continuous);
    void setAudioSampleRate(const int sampleRate);
    void setAudioBitRate(const int bitRate);
    void setAudioChannelCount(const int channelCount);
    void setAudioEncodingMode(const QtMultimediaKit::EncodingMode mode);

    // Video Options
    QSize pixelAspectRatio();
    void setPixelAspectRatio(const QSize par);
    int gain();
    void setGain(const int gain);
    int maxClipSizeInBytes() const;
    void setMaxClipSizeInBytes(const int size);

private: // Internal

    QMediaRecorder::Error fromSymbianErrorToQtMultimediaError(int aError);

    void initializeVideoCaptureSettings();
    void doInitializeVideoRecorderL();
    void commitVideoEncoderSettings();
    void queryAudioEncoderSettings();
    void queryVideoEncoderSettings();
    void validateRequestedCodecs();
    void resetSession(bool errorHandling = false);

    void doSetCodecsL();
    QString determineProfileAndLevel();
    void doSetVideoResolution(const QSize &resolution);
    void doSetFrameRate(qreal rate);
    void doSetBitrate(const int &bitrate);

    void updateVideoCaptureContainers();
    void doUpdateVideoCaptureContainersL();
    void selectController(const QString &format,
                          TUid &controllerUid,
                          TUid &formatUid);

    void doPopulateVideoCodecsDataL();
    void doPopulateVideoCodecsL();
#ifndef S60_DEVVIDEO_RECORDING_SUPPORTED
    void doPopulateMaxVideoParameters();
#endif // S60_DEVVIDEO_RECORDING_SUPPORTED
    void doPopulateAudioCodecsL();

    QList<int> doGetSupportedSampleRatesL(const QAudioEncoderSettings &settings,
                                          bool *continuous);
    QSize maximumResolutionForMimeType(const QString &mimeType) const;
    qreal maximumFrameRateForMimeType(const QString &mimeType) const;
    int maximumBitRateForMimeType(const QString &mimeType) const;

signals: // Notification Signals

    void stateChanged(S60VideoCaptureSession::TVideoCaptureState);
    void positionChanged(qint64);
    void mutedChanged(bool);
    void captureSizeChanged(const QSize&);
    void error(int, const QString&);

private slots: // Internal Slots

    void cameraStatusChanged(QCamera::Status);
    void durationTimerTriggered();

private: // Structs

    /*
     * This structure holds the information of supported video mime types for
     * the format and also description for it.
     */
    struct VideoFormatData {
        QString     description;
        QStringList supportedMimeTypes;
    };

    /*
     * This structure is used to define supported resolutions and framerate
     * (depending on each other) for each supported encoder mime type (defining
     * encoder, profile and level)
     */
    struct SupportedFrameRatePictureSize {
        SupportedFrameRatePictureSize() {}
        SupportedFrameRatePictureSize(qreal rate, QSize size):
            frameRate(rate),
            frameSize(size) {}
        qreal frameRate;
        QSize frameSize;
        };

    /*
     * This structure defines supported resolution/framerate pairs and maximum
     * bitrate for a single encodec device. It also the supported mime types
     * (codec, profile and level) of the encoder device.
     *
     * Structure defines 2 contructors:
     *    - First with no attributes
     *    - Second, which will construct the sructure appending one
     *      resolution/framerate pair to the list of
     *      SupportedFrameRatePictureSizes and setting the given bitrate as
     *      maximum. This second constructor is for convenience.
     *
     * This struct is used in m_videoParametersForEncoder (QList).
     *
     * Here's a visualization of an example strcuture:
     * STRUCT:
     *    |-- Resolution/FrameRate Pairs:
     *    |      |- VGA / 30fps
     *    |      |- 720p / 25fps
     *    |      |- Etc.
     *    |
     *    |-- MimeTypes:
     *    |      |- video/mp4v-es; profile-level-id=1
     *    |      |- video/mp4v-es; profile-level-id=2
     *    |      |- Etc.
     *    |
     *    |-- Max BitRate: 1Mbps
     */
    struct MaxResolutionRatesAndTypes {
        MaxResolutionRatesAndTypes() {}
        MaxResolutionRatesAndTypes(QSize size, qreal fRate, int bRate):
            bitRate(bRate)
        {
            frameRatePictureSizePair.append(SupportedFrameRatePictureSize(fRate,size));
        }
        QList<SupportedFrameRatePictureSize> frameRatePictureSizePair;
        QStringList                          mimeTypes;
        int                                  bitRate;
    };

private: // Data

    CCameraEngine               *m_cameraEngine;
    CVideoRecorderUtility       *m_videoRecorder;
    QTimer                      *m_durationTimer;
    qint64                      m_position;
    // Symbian ErrorCode
    mutable int                 m_error;
    // This defines whether Camera is in ActiveStatus or not
    bool                        m_cameraStarted;
    // Internal state of the video recorder
    TVideoCaptureState          m_captureState;
    // Actual output file name/path
    QUrl                        m_sink;
    // Requested output file name/path, this may be different from m_sink if
    // asynchronous operation was ongoing in the CVideoRecorderUtility when new
    // outputLocation was set.
    QUrl                        m_requestedSink;
    // Requested videoSettings. The may not be active settings before those are
    // committed (with commitVideoEncoderSettings())
    QVideoEncoderSettings       m_videoSettings;
    // Requested audioSettings. The may not be active settings before those are
    // committed (with commitVideoEncoderSettings())
    QAudioEncoderSettings       m_audioSettings;
    // Tells whether settings should be initialized when changing the camera
    bool                        m_captureSettingsSet;
    // Active container
    QString                     m_container;
    // Requested container, this may be different from m_container if
    // asynchronous operation was ongoing in the CVideoRecorderUtility when new
    // container was set.
    QString                     m_requestedContainer;
    // Requested muted value. This may not be active value before settings are
    // committed (with commitVideoEncoderSettings())
    bool                        m_muted;
    // Maximum ClipSize in Bytes
    int                         m_maxClipSize;
    // List of supported video codec mime types
    QStringList                 m_videoCodecList;
    // Hash of supported video codec mime types and corresponding FourCC codes
    QHash<QString, TFourCC>     m_audioCodecList;
    // Map of video capture controllers information. It is populated during
    // doUpdateVideoCaptureContainersL().
    //
    // Here's a visualization of an example strcuture:
    // m_videoControllerMap(HASH):
    //   |
    //   |-- Controller 1 : HASH
    //   |                   |- Container 1 (UID) : FormatData
    //   |                   |                          |- Description
    //   |                   |                          |- List of supported MimeTypes
    //   |                   |- Container 2 (UID) : FormatData
    //   |                   |                          |- Description
    //   |                   |                          |- List of supported MimeTypes
    //   |                   |- Etc.
    //   |
    //   |-- Controller 2: HASH
    //   |                   |- Container 1 (UID) : FormatData
    //   |                   |                          |- Description
    //   |                   |                          |- List of supported MimeTypes
    //   |                   |- Etc.
    //
    QHash<TInt, QHash<TInt,VideoFormatData> > m_videoControllerMap;
    // List of Encoder information. If DevVideoRecord is available info is
    // gathered during doPopulateVideoCodecsDataL() for each encoder (hw
    // accelerated and supporting camera input) found. If DevVideoRecord is not
    // available, the info is set in doPopulateMaxVideoParameters() based on
    // supported codec list received from CVideoRecorderUtility.
    QList<MaxResolutionRatesAndTypes> m_videoParametersForEncoder;
    // Set if OpenFileL should be executed when currently ongoing operation
    // is completed.
    bool                        m_openWhenReady;
    // Set if video capture should be prepared after OpenFileL has completed
    bool                        m_prepareAfterOpenComplete;
    // Set if video capture should be started when Prepare has completed
    bool                        m_startAfterPrepareComplete;
    // Tells if settings have been set after last Prepare()
    bool                        m_uncommittedSettings;
    // Tells if settings need to be applied after ongoing operation has finished
    bool                        m_commitSettingsWhenReady;
};

#endif // S60VIDEOCAPTURESESSION_H
