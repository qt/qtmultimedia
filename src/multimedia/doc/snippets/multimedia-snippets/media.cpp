// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/* Media related snippets */
#include <QFile>
#include <QTimer>
#include <QBuffer>

#include "qmediaplaylist.h"
#include "qmediarecorder.h"
#include "qplatformmediaplayer_p.h"
#include "qmediaplayer.h"
#include "qvideowidget.h"
#include "qimagecapture.h"
#include "qcamera.h"
#include "qcameraviewfinder.h"
#include "qaudiorecorder.h"
#include "qurl.h"
#include <QVideoSink>

class MediaExample : public QObject {
    Q_OBJECT

    void MediaControl();
    void MediaPlayer();
    void MediaRecorder();
    void recorderSettings();
    void imageSettings();

private:
    // Common naming
    QVideoWidget *videoWidget;
    QWidget *widget;
    QMediaPlayer *player;
    QAudioOutput *audioOutput;
    QMediaPlaylist *playlist;
    QMediaContent video;
    QMediaRecorder *recorder;
    QCamera *camera;
    QCameraViewfinder *viewfinder;
    QImageCapture *imageCapture;
    QString fileName;

    QMediaContent image1;
    QMediaContent image2;
    QMediaContent image3;
};

void MediaExample::MediaControl()
{
}


void MediaExample::recorderSettings()
{
    //! [Media recorder settings]
    QMediaFormat format(QMediaFormat::MPEG4);
    format.setVideoCodec(QMediaRecorder::VideoCodec::H264);
    format.setAudioCodec(QMediaRecorder::AudioCodec::MP3);

    recorder->setMediaFormat(settings);
    //! [Media recorder settings]
}

void MediaExample::imageSettings()
{
    //! [Image recorder settings]
    imageCapture->setFileFormat(QImageCapture::JPEG);
    imageCapture->setResolution(1600, 1200);
    //! [Image recorder settings]
}

void MediaExample::MediaPlayer()
{
    //! [Player]
    player = new QMediaPlayer;
    audioOutput = new QAudioOutput;
    player->setAudioOutput(audioOutput);
    connect(player, &QMediaPlayer::positionChanged, this, &MediaExample::positionChanged);
    player->setSource(QUrl::fromLocalFile("/Users/me/Music/coolsong.mp3"));
    audioOutput->setVolume(0.5);
    player->play();
    //! [Player]

    //! [Local playback]
    player = new QMediaPlayer;
    audioOutput = new QAudioOutput;
    player->setAudioOutput(audioOutput);
    // ...
    player->setSource(QUrl::fromLocalFile("/Users/me/Music/coolsong.mp3"));
    audioOutput->setVolume(0.5);
    player->play();
    //! [Local playback]
}

void MediaExample::MediaRecorder()
{
    //! [Media recorder]
    QMediaCaptureSession session;
    QAudioInput audioInput;
    session.setAudioInput(&input);
    QMediaRecorder recorder;
    session.setRecorder(&recorder);
    recorder.setQuality(QMediaRecorder::HighQuality);
    recorder.setOutputLocation(QUrl::fromLocalFile("test.mp3"));
    recorder.record();
    //! [Media recorder]
}

