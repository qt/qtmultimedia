/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#ifndef QFFMPEGENCODER_P_H
#define QFFMPEGENCODER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qffmpegthread_p.h"
#include "qffmpeg_p.h"
#include "qffmpeghwaccel_p.h"

#include <private/qplatformmediarecorder_p.h>
#include <qaudioformat.h>
#include <qaudiobuffer.h>

#include <qqueue.h>

QT_BEGIN_NAMESPACE

class QFFmpegAudioInput;
class QVideoFrame;
class QPlatformCamera;

namespace QFFmpeg
{

class Muxer;
class AudioEncoder;
class VideoEncoder;

class Encoder : public QObject
{
    Q_OBJECT
public:
    Encoder(const QMediaEncoderSettings &settings, const QUrl &url);
    ~Encoder();

    void addAudioInput(QFFmpegAudioInput *input);
    void addVideoSource(QPlatformCamera *source);

    void start();
    void finalize();

public Q_SLOTS:
    void newAudioBuffer(const QAudioBuffer &buffer);
    void newVideoFrame(const QVideoFrame &frame);
    void newTimeStamp(qint64 time);

Q_SIGNALS:
    void durationChanged(qint64 duration);

public:

    QMediaEncoderSettings settings;
    AVFormatContext *formatContext = nullptr;
    Muxer *muxer = nullptr;
    bool isRecording = false;

    AudioEncoder *audioEncode = nullptr;
    VideoEncoder *videoEncode = nullptr;

    QMutex timeMutex;
    qint64 timeRecorded = 0;
};


class Muxer : public Thread
{
    mutable QMutex queueMutex;
    QQueue<AVPacket *> packetQueue;
public:
    Muxer(Encoder *encoder);

    void addPacket(AVPacket *);

private:
    AVPacket *takePacket();

    void init() override;
    void cleanup() override;
    bool shouldWait() const override;
    void loop() override;

    Encoder *encoder;
};

class EncoderThread : public Thread
{
public:

protected:
    void retrievePackets();

    void cleanup() override;

    Encoder *encoder = nullptr;
    AVStream *stream = nullptr;
    AVCodecContext *codec = nullptr;
};

class AudioEncoder : public EncoderThread
{
    mutable QMutex queueMutex;
    QQueue<QAudioBuffer> audioBufferQueue;
public:
    AudioEncoder(Encoder *encoder, QFFmpegAudioInput *input, const QMediaEncoderSettings &settings);

    void addBuffer(const QAudioBuffer &buffer);

    QFFmpegAudioInput *audioInput() const { return input; }

private:
    QAudioBuffer takeBuffer();

    void init() override;
    void cleanup() override;
    bool shouldWait() const override;
    void loop() override;


    QFFmpegAudioInput *input;
    QAudioFormat format;

    SwrContext *resampler = nullptr;
    qint64 samplesWritten = 0;
};


class VideoEncoder : public EncoderThread
{
    mutable QMutex queueMutex;
    QQueue<QVideoFrame> videoFrameQueue;
public:
    VideoEncoder(Encoder *encoder, QPlatformCamera *camera, const QMediaEncoderSettings &settings);
    ~VideoEncoder();

    void addFrame(const QVideoFrame &frame);

private:
    QVideoFrame takeFrame();

    void init() override;
    void cleanup() override;
    bool shouldWait() const override;
    void loop() override;

    QMediaEncoderSettings m_encoderSettings;
    QPlatformCamera *m_camera = nullptr;

    QFFmpeg::HWAccel accel;
    SwsContext *converter = nullptr;
    qint64 baseTime = -1;
};

}

QT_END_NAMESPACE

#endif
