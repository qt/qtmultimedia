/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
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

//TESTED_COMPONENT=src/multimedia

#include <qtmultimediadefs.h>
#include <QtTest/QtTest>

#include <QtCore/qdir.h>

#include <qgraphicsvideoitem.h>
#include <qmediaimageviewer.h>
#include <private/qmediaimageviewerservice_p.h>
#include <qmediaplaylist.h>
#include <qmediaservice.h>
#include <qvideorenderercontrol.h>
#include <qvideowidget.h>
#include <qvideowidgetcontrol.h>

#include <qabstractvideosurface.h>
#include <qvideosurfaceformat.h>

QT_USE_NAMESPACE

class tst_QMediaImageViewerWidgets : public QObject
{
    Q_OBJECT
private slots:
    void setVideoOutput();
};

class QtTestVideoSurface : public QAbstractVideoSurface
{
public:
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
            QAbstractVideoBuffer::HandleType handleType) const {
        QList<QVideoFrame::PixelFormat> formats;
        if (handleType == QAbstractVideoBuffer::NoHandle) {
            formats << QVideoFrame::Format_RGB32;
        }
        return formats;
    }

    QVideoFrame frame() const { return m_frame;  }

    bool present(const QVideoFrame &frame) { m_frame = frame; return true; }

private:
    QVideoFrame m_frame;
};

void tst_QMediaImageViewerWidgets::setVideoOutput()
{
    QMediaImageViewer imageViewer;
    imageViewer.setMedia(QMediaContent(QUrl("qrc:///images/image.png")));

    connect(&imageViewer, SIGNAL(mediaStatusChanged(QMediaImageViewer::MediaStatus)),
            &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(2);

    if (imageViewer.mediaStatus() != QMediaImageViewer::LoadedMedia)
        QSKIP("failed to load test image");

    QVideoWidget widget;
    QGraphicsVideoItem item;
    QtTestVideoSurface surface;

    imageViewer.setVideoOutput(&widget);
    QVERIFY(widget.mediaObject() == &imageViewer);

    imageViewer.setVideoOutput(&item);
    QVERIFY(widget.mediaObject() == 0);
    QVERIFY(item.mediaObject() == &imageViewer);

    imageViewer.setVideoOutput(reinterpret_cast<QVideoWidget *>(0));
    QVERIFY(item.mediaObject() == 0);

    imageViewer.setVideoOutput(&widget);
    QVERIFY(widget.mediaObject() == &imageViewer);

    imageViewer.setVideoOutput(reinterpret_cast<QGraphicsVideoItem *>(0));
    QVERIFY(widget.mediaObject() == 0);

    imageViewer.setVideoOutput(&surface);
    QVERIFY(surface.isActive());

    imageViewer.setVideoOutput(reinterpret_cast<QAbstractVideoSurface *>(0));
    QVERIFY(!surface.isActive());

    imageViewer.setVideoOutput(&surface);
    QVERIFY(surface.isActive());

    imageViewer.setVideoOutput(&widget);
    QVERIFY(!surface.isActive());
    QVERIFY(widget.mediaObject() == &imageViewer);

    imageViewer.setVideoOutput(&surface);
    QVERIFY(surface.isActive());
    QVERIFY(widget.mediaObject() == 0);
}

QTEST_MAIN(tst_QMediaImageViewerWidgets)

#include "tst_qmediaimageviewerwidgets.moc"
