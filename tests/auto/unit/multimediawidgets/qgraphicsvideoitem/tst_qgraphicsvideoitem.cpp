// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtmultimediaglobal.h>
#include "qgraphicsvideoitem.h"
#include <QtTest/qtest.h>
#include <QtTest/qtesteventloop.h>
#include <QtTest/qsignalspy.h>

#include <qvideosink.h>
#include <qvideoframeformat.h>
#include <qvideoframe.h>
#include <qmediaplayer.h>

#include <QtWidgets/qapplication.h>
#include <QtWidgets/qgraphicsscene.h>
#include <QtWidgets/qgraphicsview.h>

#include <qmockintegration.h>

QT_USE_NAMESPACE

Q_ENABLE_MOCK_MULTIMEDIA_PLUGIN

class tst_QGraphicsVideoItem : public QObject
{
    Q_OBJECT
public slots:
    void initTestCase();

private slots:
    void nullObject();
    void playerDestroyed();

    void show();

    void aspectRatioMode();
    void offset();
    void size();
    void nativeSize_data();
    void nativeSize();

    void boundingRect_data();
    void boundingRect();

    void paint();
};

class QtTestGraphicsVideoItem : public QGraphicsVideoItem
{
public:
    QtTestGraphicsVideoItem(QGraphicsItem *parent = nullptr)
        : QGraphicsVideoItem(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override
    {
        ++m_paintCount;

        QTestEventLoop::instance().exitLoop();

        QGraphicsVideoItem::paint(painter, option, widget);
    }

    bool waitForPaint(int secs)
    {
        const int paintCount = m_paintCount;

        QTestEventLoop::instance().enterLoop(secs);

        return m_paintCount != paintCount;
    }

    [[nodiscard]] int paintCount() const
    {
        return m_paintCount;
    }

private:
    int m_paintCount = 0;
};

void tst_QGraphicsVideoItem::initTestCase()
{
}

void tst_QGraphicsVideoItem::nullObject()
{
    QGraphicsVideoItem item(nullptr);

    QVERIFY(item.boundingRect().isEmpty());
}

void tst_QGraphicsVideoItem::playerDestroyed()
{
    QGraphicsVideoItem item;
    {
        QMediaPlayer player;
        player.setVideoOutput(&item);
    }

    QVERIFY(item.boundingRect().isEmpty());
}

void tst_QGraphicsVideoItem::show()
{
    auto *item = new QtTestGraphicsVideoItem;
    QMediaPlayer player;
    player.setVideoOutput(item);

    // Graphics items are visible by default
    item->hide();
    item->show();

    QGraphicsScene graphicsScene;
    graphicsScene.addItem(item);
    QGraphicsView graphicsView(&graphicsScene);
    graphicsView.show();

    QVERIFY(item->paintCount() || item->waitForPaint(1));

    QVERIFY(item->boundingRect().isEmpty());

    // ### Ensure we get a correct bounding rect
//    QVideoFrameFormat format(QSize(320,240),QVideoFrameFormat::Format_RGB32);
//    QVERIFY(object.testService->rendererControl->surface()->start(format));

//    QCoreApplication::processEvents();
//    QVERIFY(!item->boundingRect().isEmpty());
}

void tst_QGraphicsVideoItem::aspectRatioMode()
{
    QGraphicsVideoItem item;

    QCOMPARE(item.aspectRatioMode(), Qt::KeepAspectRatio);

    item.setAspectRatioMode(Qt::IgnoreAspectRatio);
    QCOMPARE(item.aspectRatioMode(), Qt::IgnoreAspectRatio);

    item.setAspectRatioMode(Qt::KeepAspectRatioByExpanding);
    QCOMPARE(item.aspectRatioMode(), Qt::KeepAspectRatioByExpanding);

    item.setAspectRatioMode(Qt::KeepAspectRatio);
    QCOMPARE(item.aspectRatioMode(), Qt::KeepAspectRatio);
}

void tst_QGraphicsVideoItem::offset()
{
    QGraphicsVideoItem item;

    QCOMPARE(item.offset(), QPointF(0, 0));

    item.setOffset(QPointF(-32.4, 43.0));
    QCOMPARE(item.offset(), QPointF(-32.4, 43.0));

    item.setOffset(QPointF(1, 1));
    QCOMPARE(item.offset(), QPointF(1, 1));

    item.setOffset(QPointF(12, -30.4));
    QCOMPARE(item.offset(), QPointF(12, -30.4));

    item.setOffset(QPointF(-90.4, -75));
    QCOMPARE(item.offset(), QPointF(-90.4, -75));
}

void tst_QGraphicsVideoItem::size()
{
    QGraphicsVideoItem item;

    QCOMPARE(item.size(), QSizeF(320, 240));

    item.setSize(QSizeF(542.5, 436.3));
    QCOMPARE(item.size(), QSizeF(542.5, 436.3));

    item.setSize(QSizeF(-43, 12));
    QCOMPARE(item.size(), QSizeF(0, 0));

    item.setSize(QSizeF(54, -9));
    QCOMPARE(item.size(), QSizeF(0, 0));

    item.setSize(QSizeF(-90, -65));
    QCOMPARE(item.size(), QSizeF(0, 0));

    item.setSize(QSizeF(1000, 1000));
    QCOMPARE(item.size(), QSizeF(1000, 1000));
}

void tst_QGraphicsVideoItem::nativeSize_data()
{
    QTest::addColumn<QSize>("frameSize");
    QTest::addColumn<QRect>("viewport");
    QTest::addColumn<QSizeF>("nativeSize");

    QTest::newRow("640x480")
            << QSize(640, 480)
            << QRect(0, 0, 640, 480)
            << QSizeF(640, 480);

    QTest::newRow("800x600, (80,60, 640x480) viewport")
            << QSize(800, 600)
            << QRect(80, 60, 640, 480)
            << QSizeF(640, 480);
}

void tst_QGraphicsVideoItem::nativeSize()
{
    QFETCH(QSize, frameSize);
    QFETCH(QRect, viewport);
    QFETCH(QSizeF, nativeSize);

    QtTestGraphicsVideoItem item;
    QMediaPlayer player;
    player.setVideoOutput(&item);

    QCOMPARE(item.nativeSize(), QSizeF());

    QSignalSpy spy(&item, &QGraphicsVideoItem::nativeSizeChanged);

    QVideoFrameFormat format(frameSize, QVideoFrameFormat::Format_ARGB8888);
    format.setViewport(viewport);
    QVideoFrame frame(format);
    item.videoSink()->setVideoFrame(frame);

    QCoreApplication::processEvents();
    QCOMPARE(item.nativeSize(), nativeSize);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.last().first().toSizeF(), nativeSize);
}

void tst_QGraphicsVideoItem::boundingRect_data()
{
    QTest::addColumn<QSize>("frameSize");
    QTest::addColumn<QPointF>("offset");
    QTest::addColumn<QSizeF>("size");
    QTest::addColumn<Qt::AspectRatioMode>("aspectRatioMode");
    QTest::addColumn<QRectF>("expectedRect");


    QTest::newRow("640x480: (0,0 640x480), Keep")
            << QSize(640, 480)
            << QPointF(0, 0)
            << QSizeF(640, 480)
            << Qt::KeepAspectRatio
            << QRectF(0, 0, 640, 480);

    QTest::newRow("800x600, (0,0, 640x480), Keep")
            << QSize(800, 600)
            << QPointF(0, 0)
            << QSizeF(640, 480)
            << Qt::KeepAspectRatio
            << QRectF(0, 0, 640, 480);

    QTest::newRow("800x600, (0,0, 640x480), KeepByExpanding")
            << QSize(800, 600)
            << QPointF(0, 0)
            << QSizeF(640, 480)
            << Qt::KeepAspectRatioByExpanding
            << QRectF(0, 0, 640, 480);

    QTest::newRow("800x600, (0,0, 640x480), Ignore")
            << QSize(800, 600)
            << QPointF(0, 0)
            << QSizeF(640, 480)
            << Qt::IgnoreAspectRatio
            << QRectF(0, 0, 640, 480);

    QTest::newRow("800x600, (100,100, 640x480), Keep")
            << QSize(800, 600)
            << QPointF(100, 100)
            << QSizeF(640, 480)
            << Qt::KeepAspectRatio
            << QRectF(100, 100, 640, 480);

    QTest::newRow("800x600, (100,-100, 640x480), KeepByExpanding")
            << QSize(800, 600)
            << QPointF(100, -100)
            << QSizeF(640, 480)
            << Qt::KeepAspectRatioByExpanding
            << QRectF(100, -100, 640, 480);

    QTest::newRow("800x600, (-100,-100, 640x480), Ignore")
            << QSize(800, 600)
            << QPointF(-100, -100)
            << QSizeF(640, 480)
            << Qt::IgnoreAspectRatio
            << QRectF(-100, -100, 640, 480);

    QTest::newRow("800x600, (0,0, 1920x1024), Keep")
            << QSize(800, 600)
            << QPointF(0, 0)
            << QSizeF(1920, 1024)
            << Qt::KeepAspectRatio
            << QRectF(832.0 / 3, 0, 4096.0 / 3, 1024);

    QTest::newRow("800x600, (0,0, 1920x1024), KeepByExpanding")
            << QSize(800, 600)
            << QPointF(0, 0)
            << QSizeF(1920, 1024)
            << Qt::KeepAspectRatioByExpanding
            << QRectF(0, 0, 1920, 1024);

    QTest::newRow("800x600, (0,0, 1920x1024), Ignore")
            << QSize(800, 600)
            << QPointF(0, 0)
            << QSizeF(1920, 1024)
            << Qt::IgnoreAspectRatio
            << QRectF(0, 0, 1920, 1024);

    QTest::newRow("800x600, (100,100, 1920x1024), Keep")
            << QSize(800, 600)
            << QPointF(100, 100)
            << QSizeF(1920, 1024)
            << Qt::KeepAspectRatio
            << QRectF(100 + 832.0 / 3, 100, 4096.0 / 3, 1024);

    QTest::newRow("800x600, (100,-100, 1920x1024), KeepByExpanding")
            << QSize(800, 600)
            << QPointF(100, -100)
            << QSizeF(1920, 1024)
            << Qt::KeepAspectRatioByExpanding
            << QRectF(100, -100, 1920, 1024);

    QTest::newRow("800x600, (-100,-100, 1920x1024), Ignore")
            << QSize(800, 600)
            << QPointF(-100, -100)
            << QSizeF(1920, 1024)
            << Qt::IgnoreAspectRatio
            << QRectF(-100, -100, 1920, 1024);
}

void tst_QGraphicsVideoItem::boundingRect()
{
    QFETCH(QSize, frameSize);
    QFETCH(QPointF, offset);
    QFETCH(QSizeF, size);
    QFETCH(Qt::AspectRatioMode, aspectRatioMode);
    QFETCH(QRectF, expectedRect);

    QtTestGraphicsVideoItem item;
    QMediaPlayer player;
    player.setVideoOutput(&item);

    item.setOffset(offset);
    item.setSize(size);
    item.setAspectRatioMode(aspectRatioMode);

    QVideoFrameFormat format(frameSize, QVideoFrameFormat::Format_ARGB8888);
    QVideoFrame frame(format);
    item.videoSink()->setVideoFrame(frame);

    QCoreApplication::processEvents();
    QCOMPARE(item.boundingRect(), expectedRect);
}

static const uchar rgb32ImageData[] =
{
    0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
    0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00
};

void tst_QGraphicsVideoItem::paint()
{
    auto *item = new QtTestGraphicsVideoItem;
    QMediaPlayer player;
    player.setVideoOutput(item);

    QGraphicsScene graphicsScene;
    graphicsScene.addItem(item);
    QGraphicsView graphicsView(&graphicsScene);
    graphicsView.show();
    QVERIFY(item->paintCount() || item->waitForPaint(1));

    auto *sink = item->videoSink();
    QTEST_ASSERT(sink);

    QVideoFrameFormat format(QSize(2, 2), QVideoFrameFormat::Format_XRGB8888);
    QVideoFrame frame(format);
    frame.map(QVideoFrame::WriteOnly);
    memcpy(frame.bits(0), rgb32ImageData, frame.mappedBytes(0));
    frame.unmap();

    sink->setVideoFrame(frame);

    QVERIFY(item->waitForPaint(1));
}

QTEST_MAIN(tst_QGraphicsVideoItem)

#include "tst_qgraphicsvideoitem.moc"
