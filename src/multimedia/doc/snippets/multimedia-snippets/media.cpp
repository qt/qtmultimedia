/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Mobility Components.
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
    connect(player, SIGNAL(positionChanged(qint64)), this, SLOT(positionChanged(qint64)));
    player->setSource(QUrl::fromLocalFile("/Users/me/Music/coolsong.mp3"));
    audioOutput->setVolume(50);
    player->play();
    //! [Player]

    //! [Local playback]
    player = new QMediaPlayer;
    audioOutput = new QAudioOutput;
    player->setAudioOutput(audioOutput);
    // ...
    player->setSource(QUrl::fromLocalFile("/Users/me/Music/coolsong.mp3"));
    audioOutput->setVolume(50);
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

