/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/* Media related snippets */
#include <QFile>
#include <QTimer>

#include "qmediaplaylist.h"
#include "qmediarecorder.h"
#include "qmediaservice.h"
#include "qmediaimageviewer.h"
#include "qmediaimageviewer.h"
#include "qmediaplayercontrol.h"
#include "qmediaplayer.h"
#include "qradiotuner.h"
#include "qvideowidget.h"
#include "qcameraimagecapture.h"
#include "qcamera.h"

class MediaExample : public QObject {
    Q_OBJECT

    void MediaControl();
    void MediaImageViewer();
    void MediaPlayer();
    void RadioTuna();
    void MediaRecorder();
    void EncoderSettings();
    void ImageEncoderSettings();

private:
    // Common naming
    QMediaService *mediaService;
    QVideoWidget *videoWidget;
    QWidget *widget;
    QMediaPlayer *player;
    QMediaPlaylist *playlist;
    QMediaContent video;
    QMediaRecorder *recorder;
    QMediaImageViewer *viewer;
    QCamera *camera;
    QCameraImageCapture *imageCapture;
    QString fileName;
    QRadioTuner *radio;
    QMediaContent image1;
    QMediaContent image2;
    QMediaContent image3;

    static const int yourRadioStationFrequency = 11;
};

void MediaExample::MediaControl()
{
    {
    //! [Request control]
    QMediaPlayerControl *control = qobject_cast<QMediaPlayerControl *>(
            mediaService->requestControl("com.nokia.Qt.QMediaPlayerControl/1.0"));
    //! [Request control]
    Q_UNUSED(control);
    }

    {
    //! [Request control templated]
    QMediaPlayerControl *control = mediaService->requestControl<QMediaPlayerControl *>();
    //! [Request control templated]
    Q_UNUSED(control);
    }
}


void MediaExample::EncoderSettings()
{
    //! [Audio encoder settings]
    QAudioEncoderSettings audioSettings;
    audioSettings.setCodec("audio/mpeg");
    audioSettings.setChannelCount(2);

    recorder->setEncodingSettings(audioSettings);
    //! [Audio encoder settings]

    //! [Video encoder settings]
    QVideoEncoderSettings videoSettings;
    videoSettings.setCodec("video/mpeg2");
    videoSettings.setResolution(640, 480);

    recorder->setEncodingSettings(audioSettings, videoSettings);
    //! [Video encoder settings]
}

void MediaExample::ImageEncoderSettings()
{
    //! [Image encoder settings]
    QImageEncoderSettings imageSettings;
    imageSettings.setCodec("image/jpeg");
    imageSettings.setResolution(1600, 1200);

    imageCapture->setEncodingSettings(imageSettings);
    //! [Image encoder settings]
}

void MediaExample::MediaImageViewer()
{
    //! [Binding]
    viewer = new QMediaImageViewer(this);

    videoWidget = new QVideoWidget;
    viewer->bind(videoWidget);
    videoWidget->show();
    //! [Binding]

    //! [Playlist]
    playlist = new QMediaPlaylist(this);
    playlist->setPlaybackMode(QMediaPlaylist::Loop);
    playlist->addMedia(image1);
    playlist->addMedia(image2);
    playlist->addMedia(image3);

    viewer->setPlaylist(playlist);
    viewer->setTimeout(5000);
    viewer->play();
    //! [Playlist]
}

void MediaExample::MediaPlayer()
{
    //! [Player]
    player = new QMediaPlayer;
    connect(player, SIGNAL(positionChanged(qint64)), this, SLOT(positionChanged(qint64)));
    player->setMedia(QUrl::fromLocalFile("/Users/me/Music/coolsong.mp3"));
    player->setVolume(50);
    player->play();
    //! [Player]

    //! [Local playback]
    player = new QMediaPlayer;
    // ...
    player->setMedia(QUrl::fromLocalFile("/Users/me/Music/coolsong.mp3"));
    player->setVolume(50);
    player->play();
    //! [Local playback]

    //! [Audio playlist]
    player = new QMediaPlayer;

    playlist = new QMediaPlaylist(player);
    playlist->addMedia(QUrl("http://example.com/myfile1.mp3"));
    playlist->addMedia(QUrl("http://example.com/myfile2.mp3"));
    // ...
    playlist->setCurrentIndex(1);
    player->play();
    //! [Audio playlist]

    //! [Movie playlist]
    playlist = new QMediaPlaylist;
    playlist->addMedia(QUrl("http://example.com/movie1.mp4"));
    playlist->addMedia(QUrl("http://example.com/movie2.mp4"));
    playlist->addMedia(QUrl("http://example.com/movie3.mp4"));
    playlist->setCurrentIndex(1);

    player = new QMediaPlayer;
    player->setPlaylist(playlist);

    videoWidget = new QVideoWidget;
    player->setVideoOutput(videoWidget);
    videoWidget->show();

    player->play();
    //! [Movie playlist]
}

void MediaExample::MediaRecorder()
{
    //! [Media recorder]
    // Audio only recording
    recorder = new QMediaRecorder(camera);

    QAudioEncoderSettings audioSettings;
    audioSettings.setCodec("audio/amr");
    audioSettings.setQuality(QtMultimedia::HighQuality);

    recorder->setEncodingSettings(audioSettings);

    recorder->setOutputLocation(QUrl::fromLocalFile(fileName));
    recorder->record();
    //! [Media recorder]

#if 0
    //! [Audio recorder]
    audioRecorder = new QAudioRecorder;

    QAudioEncoderSettings audioSettings;
    audioSettings.setCodec("audio/amr");
    audioSettings.setQuality(QtMultimedia::HighQuality);

    audioRecorder->setEncodingSettings(audioSettings);

    audioRecorder->setOutputLocation(QUrl::fromLocalFile("test.amr"));
    audioRecorder->record();
    //! [Audio recorder]
#endif
}

void MediaExample::RadioTuna()
{
    //! [Radio tuner]
    radio = new QRadioTuner;
    connect(radio, SIGNAL(frequencyChanged(int)), this, SLOT(freqChanged(int)));
    if (radio->isBandSupported(QRadioTuner::FM)) {
        radio->setBand(QRadioTuner::FM);
        radio->setFrequency(yourRadioStationFrequency);
        radio->setVolume(100);
        radio->start();
    }
    //! [Radio tuner]
}


