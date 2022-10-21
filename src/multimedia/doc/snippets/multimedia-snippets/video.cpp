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
