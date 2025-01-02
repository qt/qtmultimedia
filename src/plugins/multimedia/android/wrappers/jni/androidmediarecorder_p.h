// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROIDMEDIARECORDER_H
#define ANDROIDMEDIARECORDER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qobject.h>
#include <QtCore/qjniobject.h>
#include <qsize.h>

QT_BEGIN_NAMESPACE

class AndroidCamera;
class AndroidSurfaceTexture;
class AndroidSurfaceHolder;

class AndroidCamcorderProfile
{
public:
    enum Quality { // Needs to match CamcorderProfile
        QUALITY_LOW,
        QUALITY_HIGH,
        QUALITY_QCIF,
        QUALITY_CIF,
        QUALITY_480P,
        QUALITY_720P,
        QUALITY_1080P,
        QUALITY_QVGA
    };

    enum Field {
        audioBitRate,
        audioChannels,
        audioCodec,
        audioSampleRate,
        duration,
        fileFormat,
        quality,
        videoBitRate,
        videoCodec,
        videoFrameHeight,
        videoFrameRate,
        videoFrameWidth
    };

    static bool hasProfile(jint cameraId, Quality quality);
    static AndroidCamcorderProfile get(jint cameraId, Quality quality);
    int getValue(Field field) const;

private:
    AndroidCamcorderProfile(const QJniObject &camcorderProfile);
    QJniObject m_camcorderProfile;
};

class AndroidMediaRecorder : public QObject
{
    Q_OBJECT
public:
    enum AudioEncoder {
        DefaultAudioEncoder = 0,
        AMR_NB_Encoder = 1,
        AMR_WB_Encoder = 2,
        AAC = 3,
        OPUS = 7,
        VORBIS = 6
    };

    enum AudioSource {
        DefaultAudioSource = 0,
        Mic = 1,
        VoiceUplink = 2,
        VoiceDownlink = 3,
        VoiceCall = 4,
        Camcorder = 5,
        VoiceRecognition = 6
    };

    enum VideoEncoder {
        DefaultVideoEncoder = 0,
        H263 = 1,
        H264 = 2,
        MPEG_4_SP = 3,
        HEVC = 5
    };

    enum VideoSource {
        DefaultVideoSource = 0,
        Camera = 1
    };

    enum OutputFormat {
        DefaultOutputFormat = 0,
        THREE_GPP = 1,
        MPEG_4 = 2,
        AMR_NB_Format = 3,
        AMR_WB_Format = 4,
        AAC_ADTS = 6,
        OGG = 11,
        WEBM = 9
    };

    AndroidMediaRecorder();
    ~AndroidMediaRecorder();

    void release();
    bool prepare();
    void reset();

    bool start();
    void stop();

    void setAudioChannels(int numChannels);
    void setAudioEncoder(AudioEncoder encoder);
    void setAudioEncodingBitRate(int bitRate);
    void setAudioSamplingRate(int samplingRate);
    void setAudioSource(AudioSource source);
    bool isAudioSourceSet() const;
    bool setAudioInput(const QByteArray &id);

    void setCamera(AndroidCamera *camera);
    void setVideoEncoder(VideoEncoder encoder);
    void setVideoEncodingBitRate(int bitRate);
    void setVideoFrameRate(int rate);
    void setVideoSize(const QSize &size);
    void setVideoSource(VideoSource source);

    void setOrientationHint(int degrees);

    void setOutputFormat(OutputFormat format);
    void setOutputFile(const QString &path);

    void setSurfaceTexture(AndroidSurfaceTexture *texture);
    void setSurfaceHolder(AndroidSurfaceHolder *holder);

    static bool registerNativeMethods();

Q_SIGNALS:
    void error(int what, int extra);
    void info(int what, int extra);

private:
    jlong m_id;
    QJniObject m_mediaRecorder;
    bool m_isAudioSourceSet = false;
    bool m_isVideoSourceSet = false;
};

QT_END_NAMESPACE

#endif // ANDROIDMEDIARECORDER_H
