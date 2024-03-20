// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/* Video related snippets */
#include "qvideorenderercontrol.h"
#include "qmediaplayer.h"
#include "qvideosink.h"
#include "qvideowindowcontrol.h"
#include "qgraphicsvideoitem.h"
#include "qvideoframeformat.h"

#include <QFormLayout>
#include <QGraphicsView>

//! [Video producer]
class MyVideoProducer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVideoSink *videoSink READ videoSink WRITE setVideoSink)

public:
    QVideoSink* videoSink() const { return m_sink; }

    void setVideoSink(QVideoSink *sink)
    {
        m_sink = sink;
    }

    // ...

public slots:
    void onNewVideoContentReceived(const QVideoFrame &frame)
    {
        if (m_sink)
            m_sink->setVideoFrame(frame);
    }

private:
    QVideoSink *m_sink;
};

//! [Video producer]


class VideoExample : public QObject {
    Q_OBJECT
public:
    void VideoGraphicsItem();
    void VideoWidget();
    void VideoWindowControl();
    void VideoWidgetControl();
    void VideoSurface();

private:
    // Common naming
    QVideoWidget *videoWidget;
    QWidget *widget;
    QFormLayout *layout;
    QVideoSink *myVideoSink;
    QMediaPlayer *player;
    QMediaContent video;
    QGraphicsView *graphicsView;
};

void VideoExample::VideoWidget()
{
    //! [Video widget]
    player = new QMediaPlayer;
    player->setSource(QUrl("http://example.com/myclip1.mp4"));

    videoWidget = new QVideoWidget;
    player->setVideoOutput(videoWidget);

    videoWidget->show();
    player->play();
    //! [Video widget]

    player->stop();

    //! [Setting surface in player]
    player->setVideoOutput(myVideoSink);
    //! [Setting surface in player]
}

void VideoExample::VideoSurface()
{
    //! [Widget Surface]
    QImage img = QImage("images/qt-logo.png").convertToFormat(QImage::Format_ARGB32);
    QVideoFrameFormat format(img.size(), QVideoFrameFormat::Format_ARGB8888);
    videoWidget = new QVideoWidget;
    videoWidget->videoSurface()->start(format);
    videoWidget->videoSurface()->present(img);
    videoWidget->show();
    //! [Widget Surface]

    //! [GraphicsVideoItem Surface]
    QGraphicsVideoItem *item = new QGraphicsVideoItem;
    graphicsView->scene()->addItem(item);
    graphicsView->show();
    QImage img = QImage("images/qt-logo.png").convertToFormat(QImage::Format_ARGB32);
    item->videoSink()->setVideoFrame(QVideoFrame(img));
    //! [GraphicsVideoItem Surface]
}

void VideoExample::VideoGraphicsItem()
{
    //! [Video graphics item]
    player = new QMediaPlayer(this);

    QGraphicsVideoItem *item = new QGraphicsVideoItem;
    player->setVideoOutput(item);
    graphicsView->scene()->addItem(item);
    graphicsView->show();

    player->setSource(QUrl("http://example.com/myclip4.ogv"));
    player->play();
    //! [Video graphics item]
}
