/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

/* Video related snippets */
#include "qvideorenderercontrol.h"
#include "qmediaservice.h"
#include "qmediaplayer.h"
#include "qabstractvideosurface.h"
#include "qvideowidgetcontrol.h"
#include "qvideowindowcontrol.h"
#include "qgraphicsvideoitem.h"

#include <QFormLayout>
#include <QGraphicsView>

class VideoExample : public QObject {
    Q_OBJECT
public:
    void VideoGraphicsItem();
    void VideoRendererControl();
    void VideoWidget();
    void VideoWindowControl();
    void VideoWidgetControl();

private:
    // Common naming
    QMediaService *mediaService;
    QVideoWidget *videoWidget;
    QWidget *widget;
    QFormLayout *layout;
    QAbstractVideoSurface *myVideoSurface;
    QMediaPlayer *player;
    QMediaContent video;
    QGraphicsView *graphicsView;
};

void VideoExample::VideoRendererControl()
{
    //! [Video renderer control]
    QVideoRendererControl *rendererControl = mediaService->requestControl<QVideoRendererControl *>();
    rendererControl->setSurface(myVideoSurface);
    //! [Video renderer control]
}

void VideoExample::VideoWidget()
{
    //! [Video widget]
    player = new QMediaPlayer;

    videoWidget = new QVideoWidget;

    player->setVideoOutput(videoWidget);
    player->setMedia(QUrl("http://example.com/movie.mp4"));

    videoWidget->show();
    player->play();
    //! [Video widget]
}

void VideoExample::VideoWidgetControl()
{
    //! [Video widget control]
    QVideoWidgetControl *widgetControl = mediaService->requestControl<QVideoWidgetControl *>();
    layout->addWidget(widgetControl->videoWidget());
    //! [Video widget control]
}

void VideoExample::VideoWindowControl()
{
    //! [Video window control]
    QVideoWindowControl *windowControl = mediaService->requestControl<QVideoWindowControl *>();
    windowControl->setWinId(widget->winId());
    windowControl->setDisplayRect(widget->rect());
    windowControl->setAspectRatioMode(Qt::KeepAspectRatio);
    //! [Video window control]
}

void VideoExample::VideoGraphicsItem()
{
    //! [Video graphics item]
    player = new QMediaPlayer(this);

    QGraphicsVideoItem *item = new QGraphicsVideoItem;
    player->setVideoOutput(item);
    graphicsView->scene()->addItem(item);
    graphicsView->show();

    player->setMedia(video);
    player->play();
    //! [Video graphics item]
}
