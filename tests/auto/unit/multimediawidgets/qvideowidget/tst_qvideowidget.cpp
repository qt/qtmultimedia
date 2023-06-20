// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//TESTED_COMPONENT=src/multimedia

#include <qtmultimediaglobal.h>
#include <QtTest/QtTest>

#include "qvideowidget.h"
#include "qvideosink.h"
#include "qmediaplayer.h"

#include <qvideoframeformat.h>
#include <qvideoframe.h>

#include <QtWidgets/qapplication.h>

#include <qmockintegration.h>
#include <qmockvideosink.h>

QT_USE_NAMESPACE
class tst_QVideoWidget : public QObject
{
    Q_OBJECT
public slots:
    void initTestCase();

private slots:
    void nullObject();

    void show();
    void fullScreen();
    void aspectRatio();
    void sizeHint_data();
    void sizeHint();
#if 0
    void brightness_data() { color_data(); }
    void brightness();
    void contrast_data() { color_data(); }
    void contrast();
    void hue_data() { color_data(); }
    void hue();
    void saturation_data() { color_data(); }
    void saturation();
#endif

    void paint();

private:
//    void color_data();
};

class QtTestVideoWidget : public QVideoWidget
{
public:
    QtTestVideoWidget(QWidget *parent = nullptr)
        : QVideoWidget(parent)
    {
        resize(320, 240);
    }
};

void tst_QVideoWidget::initTestCase()
{
#ifdef Q_OS_MACOS
    if (qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci")
        QSKIP("SKIP on macOS CI since metal is not supported, otherwise it often crashes. To be "
              "fixed.");
#endif
}

void tst_QVideoWidget::nullObject()
{
    QtTestVideoWidget widget;

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    widget.setFullScreen(true);
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), true);

    widget.setAspectRatioMode(Qt::IgnoreAspectRatio);
    QCOMPARE(widget.aspectRatioMode(), Qt::IgnoreAspectRatio);
}



void tst_QVideoWidget::show()
{
    QtTestVideoWidget widget;

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    widget.resize(640, 480);
    QCOMPARE(widget.size(), QSize(640, 480));

    widget.move(10, 10);
    QCOMPARE(widget.size(), QSize(640, 480));

    widget.hide();
}

void tst_QVideoWidget::aspectRatio()
{
    QtTestVideoWidget widget;
    QMediaPlayer player;
    player.setVideoOutput(&widget);

    // Test the aspect ratio defaults to keeping the aspect ratio.
    QCOMPARE(widget.aspectRatioMode(), Qt::KeepAspectRatio);

    // Test the control has been informed of the aspect ratio change, post show.
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.aspectRatioMode(), Qt::KeepAspectRatio);

    // Test an aspect ratio change is enforced immediately while visible.
    widget.setAspectRatioMode(Qt::IgnoreAspectRatio);
    QCOMPARE(widget.aspectRatioMode(), Qt::IgnoreAspectRatio);

    // Test an aspect ratio set while not visible is respected.
    widget.hide();
    widget.setAspectRatioMode(Qt::KeepAspectRatio);
    QCOMPARE(widget.aspectRatioMode(), Qt::KeepAspectRatio);
    widget.show();
    QCOMPARE(widget.aspectRatioMode(), Qt::KeepAspectRatio);
}

void tst_QVideoWidget::sizeHint_data()
{
#ifdef Q_OS_MAC
    QSKIP("QTBUG-26481 - Crashes on Mac");
#endif

    QTest::addColumn<QSize>("frameSize");
    QTest::addColumn<QRect>("viewport");
    QTest::addColumn<QSize>("expectedSize");

    QTest::newRow("640x480")
            << QSize(640, 480)
            << QRect(0, 0, 640, 480)
            << QSize(640, 480);

//    QTest::newRow("800x600, (80,60, 640x480) viewport")
//            << QSize(800, 600)
//            << QRect(80, 60, 640, 480)
//            << QSize(640, 480);
}

void tst_QVideoWidget::sizeHint()
{
#ifdef Q_OS_MAC
    QSKIP("QTBUG-26481 - Crashes on Mac");
#endif

    QFETCH(QSize, frameSize);
//    QFETCH(QRect, viewport);
    QFETCH(QSize, expectedSize);

    QMockIntegrationFactory mockCreator;
    QtTestVideoWidget widget;
    QMediaPlayer player;

    player.setVideoOutput(&widget);
    auto mockSink = QMockIntegration::instance()->lastVideoSink();

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    mockSink->setNativeSize(frameSize);

    QCOMPARE(widget.sizeHint(), expectedSize);
}


void tst_QVideoWidget::fullScreen()
{
    QtTestVideoWidget widget;
    QMediaPlayer player;
    player.setVideoOutput(&widget);
    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    Qt::WindowFlags windowFlags = widget.windowFlags();

    QSignalSpy spy(&widget, SIGNAL(fullScreenChanged(bool)));

    // Test showing full screen with setFullScreen(true).
    widget.setFullScreen(true);
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), true);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.value(0).value(0).toBool(), true);

    // Test returning to normal with setFullScreen(false).
    widget.setFullScreen(false);
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.size(), 2);
    QCOMPARE(spy.value(1).value(0).toBool(), false);
    QCOMPARE(widget.windowFlags(), windowFlags);

    // Test showing full screen with showFullScreen().
    widget.showFullScreen();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), true);
    QCOMPARE(spy.size(), 3);
    QCOMPARE(spy.value(2).value(0).toBool(), true);

    // Test returning to normal with showNormal().
    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.size(), 4);
    QCOMPARE(spy.value(3).value(0).toBool(), false);
    QCOMPARE(widget.windowFlags(), windowFlags);

    // Test setFullScreen(false) and showNormal() do nothing when isFullScreen() == false.
    widget.setFullScreen(false);
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.size(), 4);
    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.size(), 4);

    // Test setFullScreen(true) and showFullScreen() do nothing when isFullScreen() == true.
    widget.showFullScreen();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    widget.setFullScreen(true);
    QCOMPARE(widget.isFullScreen(), true);
    QCOMPARE(spy.size(), 5);
    widget.showFullScreen();
    QCOMPARE(widget.isFullScreen(), true);
    QCOMPARE(spy.size(), 5);
}

static const uchar rgb32ImageData[] =
{
    0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
    0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00
};

void tst_QVideoWidget::paint()
{
    QtTestVideoWidget widget;
    QMediaPlayer player;
    player.setVideoOutput(&widget);
    widget.resize(640,480);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QVideoFrameFormat format(QSize(2, 2), QVideoFrameFormat::Format_XRGB8888);
    QVideoFrame frame(format);
    QVERIFY(frame.map(QVideoFrame::ReadWrite));
    uchar *data = frame.bits(0);
    memcpy(data, rgb32ImageData, sizeof(rgb32ImageData));
    frame.unmap();

    auto *sink = widget.videoSink();
    emit sink->setVideoFrame(frame);

    QCoreApplication::processEvents(QEventLoop::AllEvents);
}

QTEST_MAIN(tst_QVideoWidget)

#include "tst_qvideowidget.moc"
