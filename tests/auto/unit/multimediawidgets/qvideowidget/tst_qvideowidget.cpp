/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//TESTED_COMPONENT=src/multimedia

#include <qtmultimediaglobal.h>
#include <QtTest/QtTest>

#include "qvideowidget.h"
#include "qvideosink.h"
#include "qmediaplayer.h"

#include <qvideoframeformat.h>
#include <qvideoframe.h>

#include <QtWidgets/qapplication.h>

#include <qmockintegration_p.h>
#include <qmockvideosink.h>

QT_USE_NAMESPACE
class tst_QVideoWidget : public QObject
{
    Q_OBJECT
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

#if 0
    {
        QSignalSpy spy(&widget, SIGNAL(brightnessChanged(float)));

        widget.setBrightness(1);
        QCOMPARE(widget.brightness(), 1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.value(0).value(0).toFloat(), 1.);

        widget.setBrightness(1);
        QCOMPARE(widget.brightness(), 1);
        QCOMPARE(spy.count(), 1);

        widget.setBrightness(-1.2);
        QCOMPARE(widget.brightness(), -1.);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.value(1).value(0).toFloat(), -1.);
    } {
        QSignalSpy spy(&widget, SIGNAL(contrastChanged(float)));

        widget.setContrast(1.);
        QCOMPARE(widget.contrast(), 1.);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.value(0).value(0).toFloat(), 1.);

        widget.setContrast(1.);
        QCOMPARE(widget.contrast(), 1.);
        QCOMPARE(spy.count(), 1);

        widget.setContrast(-1.2);
        QCOMPARE(widget.contrast(), -1.);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.value(1).value(0).toFloat(), -1.);
    } {
        QSignalSpy spy(&widget, SIGNAL(hueChanged(float)));

        widget.setHue(1.);
        QCOMPARE(widget.hue(), 1.);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.value(0).value(0).toFloat(), 1.);

        widget.setHue(1.);
        QCOMPARE(widget.hue(), 1.);
        QCOMPARE(spy.count(), 1);

        widget.setHue(-1.2);
        QCOMPARE(widget.hue(), -1.);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.value(1).value(0).toFloat(), -1.);
    } {
        QSignalSpy spy(&widget, SIGNAL(saturationChanged(float)));

        widget.setSaturation(1.);
        QCOMPARE(widget.saturation(), 1.);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.value(0).value(0).toFloat(), 1.);

        widget.setSaturation(1.);
        QCOMPARE(widget.saturation(), 1.);
        QCOMPARE(spy.count(), 1);

        widget.setSaturation(-1.2);
        QCOMPARE(widget.saturation(), -1.);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.value(1).value(0).toFloat(), -1.);
    }
#endif
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
    QMediaPlayer player;
    QtTestVideoWidget widget;
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

    QMockIntegration mock;
    QMediaPlayer player;
    QtTestVideoWidget widget;
    player.setVideoOutput(&widget);
    auto mockSink = mock.lastVideoSink();

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    mockSink->setVideoSize(frameSize);

    QCOMPARE(widget.sizeHint(), expectedSize);
}


void tst_QVideoWidget::fullScreen()
{
    QMediaPlayer player;
    QtTestVideoWidget widget;
    player.setVideoOutput(&widget);
    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    Qt::WindowFlags windowFlags = widget.windowFlags();

    QSignalSpy spy(&widget, SIGNAL(fullScreenChanged(bool)));

    // Test showing full screen with setFullScreen(true).
    widget.setFullScreen(true);
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.value(0).value(0).toBool(), true);

    // Test returning to normal with setFullScreen(false).
    widget.setFullScreen(false);
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.value(1).value(0).toBool(), false);
    QCOMPARE(widget.windowFlags(), windowFlags);

    // Test showing full screen with showFullScreen().
    widget.showFullScreen();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), true);
    QCOMPARE(spy.count(), 3);
    QCOMPARE(spy.value(2).value(0).toBool(), true);

    // Test returning to normal with showNormal().
    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.count(), 4);
    QCOMPARE(spy.value(3).value(0).toBool(), false);
    QCOMPARE(widget.windowFlags(), windowFlags);

    // Test setFullScreen(false) and showNormal() do nothing when isFullScreen() == false.
    widget.setFullScreen(false);
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.count(), 4);
    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.count(), 4);

    // Test setFullScreen(true) and showFullScreen() do nothing when isFullScreen() == true.
    widget.showFullScreen();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    widget.setFullScreen(true);
    QCOMPARE(widget.isFullScreen(), true);
    QCOMPARE(spy.count(), 5);
    widget.showFullScreen();
    QCOMPARE(widget.isFullScreen(), true);
    QCOMPARE(spy.count(), 5);
}

#if 0
void tst_QVideoWidget::color_data()
{
    QTest::addColumn<float>("controlValue");
    QTest::addColumn<float>("value");
    QTest::addColumn<float>("expectedValue");

    QTest::newRow(".12")
            << 0.f
            << .12f
            << .12f;
    QTest::newRow("-.56")
            << .87f
            << -.56f
            << -.56f;
    QTest::newRow("100")
            << .32f
            << 1.f
            << 1.f;
    QTest::newRow("12.94")
            << 0.f
            << 12.94f
            << 1.f;
    QTest::newRow("-1.02")
            << .34f
            << -1.02f
            << -1.00f;
}

void tst_QVideoWidget::brightness()
{
    QFETCH(float, controlValue);
    QFETCH(float, value);
    QFETCH(float, expectedValue);

    QMediaPlayer player;
    QtTestVideoWidget widget;
    player.setVideoOutput(&widget);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    // Test that values start at their default
    QCOMPARE(widget.brightness(), 0);

    QSignalSpy spy(&widget, SIGNAL(brightnessChanged(float)));

    // Test the video widget sets the brightness value, bounded if necessary and emits a changed
    // signal.
    widget.setBrightness(value);
    QCOMPARE(widget.brightness(), expectedValue);
    QCOMPARE(widget.videoSink()->brightness(), expectedValue);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.value(0).value(0).toFloat(), expectedValue);

    // Test the changed signal isn't emitted if the value is unchanged.
    widget.setBrightness(value);
    QCOMPARE(widget.brightness(), expectedValue);
    QCOMPARE(widget.videoSink()->brightness(), expectedValue);
    QCOMPARE(spy.count(), 1);

    // Test the changed signal is emitted if the brightness is changed internally.
    widget.videoSink()->setBrightness(controlValue);
    QCOMPARE(widget.brightness(), controlValue);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.value(1).value(0).toFloat(), controlValue);
}

void tst_QVideoWidget::contrast()
{
    QFETCH(float, controlValue);
    QFETCH(float, value);
    QFETCH(float, expectedValue);

    QMediaPlayer player;
    QtTestVideoWidget widget;
    player.setVideoOutput(&widget);

    QCOMPARE(widget.contrast(), 0);

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.contrast(), 0);

    QSignalSpy spy(&widget, SIGNAL(contrastChanged(float)));

    widget.setContrast(value);
    QCOMPARE(widget.contrast(), expectedValue);
    QCOMPARE(widget.videoSink()->contrast(), expectedValue);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.value(0).value(0).toFloat(), expectedValue);

    widget.setContrast(value);
    QCOMPARE(widget.contrast(), expectedValue);
    QCOMPARE(widget.videoSink()->contrast(), expectedValue);
    QCOMPARE(spy.count(), 1);

    widget.videoSink()->setContrast(controlValue);
    QCOMPARE(widget.contrast(), controlValue);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.value(1).value(0).toFloat(), controlValue);
}

void tst_QVideoWidget::hue()
{
    QFETCH(float, controlValue);
    QFETCH(float, value);
    QFETCH(float, expectedValue);

    QMediaPlayer player;
    QtTestVideoWidget widget;
    player.setVideoOutput(&widget);

    QCOMPARE(widget.hue(), 0);

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.hue(), 0);

    QSignalSpy spy(&widget, SIGNAL(hueChanged(float)));

    widget.setHue(value);
    QCOMPARE(widget.hue(), expectedValue);
    QCOMPARE(widget.videoSink()->hue(), expectedValue);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.value(0).value(0).toFloat(), expectedValue);

    widget.setHue(value);
    QCOMPARE(widget.hue(), expectedValue);
    QCOMPARE(widget.videoSink()->hue(), expectedValue);
    QCOMPARE(spy.count(), 1);

    widget.videoSink()->setHue(controlValue);
    QCOMPARE(widget.hue(), controlValue);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.value(1).value(0).toFloat(), controlValue);
}

void tst_QVideoWidget::saturation()
{
    QFETCH(float, controlValue);
    QFETCH(float, value);
    QFETCH(float, expectedValue);

    QMediaPlayer player;
    QtTestVideoWidget widget;
    player.setVideoOutput(&widget);

    QCOMPARE(widget.saturation(), 0);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.saturation(), 0);

    QSignalSpy spy(&widget, SIGNAL(saturationChanged(float)));

    widget.setSaturation(value);
    QCOMPARE(widget.saturation(), expectedValue);
    QCOMPARE(widget.videoSink()->saturation(), expectedValue);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.value(0).value(0).toFloat(), expectedValue);

    widget.setSaturation(value);
    QCOMPARE(widget.saturation(), expectedValue);
    QCOMPARE(widget.videoSink()->saturation(), expectedValue);
    QCOMPARE(spy.count(), 1);

    widget.videoSink()->setSaturation(controlValue);
    QCOMPARE(widget.saturation(), controlValue);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.value(1).value(0).toFloat(), controlValue);
}
#endif

static const uchar rgb32ImageData[] =
{
    0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
    0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00
};

void tst_QVideoWidget::paint()
{
    QMediaPlayer player;
    QtTestVideoWidget widget;
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
    emit sink->newVideoFrame(frame);

    QCoreApplication::processEvents(QEventLoop::AllEvents);
}

QTEST_MAIN(tst_QVideoWidget)

#include "tst_qvideowidget.moc"
