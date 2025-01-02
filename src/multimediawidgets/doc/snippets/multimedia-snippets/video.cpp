// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// Video related snippets
// Extracted from src/multimedia/doc/snippets/multimedia-snippets/video.cpp
#include "qmediaplayer.h"
#include "qvideowidgetcontrol.h"
#include "qvideowindowcontrol.h"
#include "qgraphicsvideoitem.h"
#include "qmediaplaylist.h"

#include <QFormLayout>
#include <QGraphicsView>

class VideoExample : public QObject {
    Q_OBJECT
public:
    void VideoGraphicsItem();
    void VideoWidget();

private:
    // Common naming
    QMediaPlaylist *playlist;
    QVideoWidget *videoWidget;
    QFormLayout *layout;
    QMediaPlayer *player;
    QGraphicsView *graphicsView;
};

void VideoExample::VideoWidget()
{
    //! [Video widget]
    player = new QMediaPlayer;

    playlist = new QMediaPlaylist(player);
    playlist->addMedia(QUrl("http://example.com/myclip1.mp4"));
    playlist->addMedia(QUrl("http://example.com/myclip2.mp4"));

    videoWidget = new QVideoWidget;
    player->setVideoOutput(videoWidget);

    videoWidget->show();
    playlist->setCurrentIndex(1);
    player->play();
    //! [Video widget]

    player->stop();
}

void VideoExample::VideoGraphicsItem()
{
    //! [Video graphics item]
    player = new QMediaPlayer(this);

    QGraphicsVideoItem *item = new QGraphicsVideoItem;
    player->setVideoOutput(item);
    graphicsView->scene()->addItem(item);
    graphicsView->show();

    player->setMedia(QUrl("http://example.com/myclip4.ogv"));
    player->play();
    //! [Video graphics item]
}
