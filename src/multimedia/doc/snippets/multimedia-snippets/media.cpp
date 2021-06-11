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
#include "qcameraimagecapture.h"
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
    void AudioRecorder();
    void EncoderSettings();
    void ImageEncoderSettings();

private:
    // Common naming
    QVideoWidget *videoWidget;
    QWidget *widget;
    QMediaPlayer *player;
    QMediaPlaylist *playlist;
    QMediaContent video;
    QMediaRecorder *recorder;
    QCamera *camera;
    QCameraViewfinder *viewfinder;
    QCameraImageCapture *imageCapture;
    QString fileName;

    QMediaContent image1;
    QMediaContent image2;
    QMediaContent image3;
};

void MediaExample::MediaControl()
{
}


void MediaExample::EncoderSettings()
{
    //! [Media encoder settings]
    QMediaFormat format(QMediaFormat::MPEG4);
    format.setVideoCodec(QMediaEncoderSettings::VideoCodec::H264);
    format.setAudioCodec(QMediaEncoderSettings::AudioCodec::MP3);
    QMediaEncoderSettings settings(format);

    recorder->setEncoderSettings(settings);
    //! [Media encoder settings]
}

void MediaExample::ImageEncoderSettings()
{
    //! [Image encoder settings]
    QImageEncoderSettings imageSettings;
    imageSettings.setFormat(QCameraImageCapture::JPEG);
    imageSettings.setResolution(1600, 1200);

    imageCapture->setEncodingSettings(imageSettings);
    //! [Image encoder settings]
}

void MediaExample::MediaPlayer()
{
    //! [Player]
    player = new QMediaPlayer;
    connect(player, SIGNAL(positionChanged(qint64)), this, SLOT(positionChanged(qint64)));
    player->setSource(QUrl::fromLocalFile("/Users/me/Music/coolsong.mp3"));
    player->setVolume(50);
    player->play();
    //! [Player]

    //! [Local playback]
    player = new QMediaPlayer;
    // ...
    player->setSource(QUrl::fromLocalFile("/Users/me/Music/coolsong.mp3"));
    player->setVolume(50);
    player->play();
    //! [Local playback]
}

void MediaExample::MediaRecorder()
{
    //! [Media recorder]
    recorder = new QMediaRecorder(camera);

    QMediaEncoderSettings audioSettings(QMediaFormat::MP3);
    audioSettings.setQuality(QMediaEncoderSettings::HighQuality);

    recorder->setAudioSettings(audioSettings);

    recorder->setOutputLocation(QUrl::fromLocalFile(fileName));
    recorder->record();
    //! [Media recorder]
}

void MediaExample::AudioRecorder()
{
    //! [Audio recorder]
    QMediaRecorder recorder;
    recorder.setCaptureMode(QMediaRecorder::AudioOnly);

    QMediaEncoderSettings audioSettings(QMediaFormat::MP3);
    audioSettings.setQuality(QMediaEncoderSettings::HighQuality);

    recorder.setEncoderSettings(audioSettings);

    recorder.setOutputLocation(QUrl::fromLocalFile("test.amr"));
    recorder.record();
    //! [Audio recorder]
}

