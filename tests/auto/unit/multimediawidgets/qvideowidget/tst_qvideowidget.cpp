/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmediasource.h"
#include "qmediaservice.h"
#include <private/qpaintervideosurface_p.h>

#include <qvideosurfaceformat.h>

#include <QtWidgets/qapplication.h>

###################################### FIXME: Tests need to be rewritten!!!!!

QT_USE_NAMESPACE
class tst_QVideoWidget : public QObject
{
    Q_OBJECT
private slots:
    void nullObject();
    void noOutputs();
    void serviceDestroyed();

    void showWindowControl();
    void fullScreenWindowControl();
    void aspectRatioWindowControl();
    void sizeHintWindowControl_data() { sizeHint_data(); }
    void sizeHintWindowControl();
    void brightnessWindowControl_data() { color_data(); }
    void brightnessWindowControl();
    void contrastWindowControl_data() { color_data(); }
    void contrastWindowControl();
    void hueWindowControl_data() { color_data(); }
    void hueWindowControl();
    void saturationWindowControl_data() { color_data(); }
    void saturationWindowControl();

    void paintRendererControl();
    void paintSurface();

private:
    void sizeHint_data();
    void color_data();
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

    QVERIFY(widget.sizeHint().isEmpty());

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    widget.setFullScreen(true);
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), true);

    widget.setAspectRatioMode(Qt::IgnoreAspectRatio);
    QCOMPARE(widget.aspectRatioMode(), Qt::IgnoreAspectRatio);

    {
        QSignalSpy spy(&widget, SIGNAL(brightnessChanged(int)));

        widget.setBrightness(100);
        QCOMPARE(widget.brightness(), 100);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.value(0).value(0).toInt(), 100);

        widget.setBrightness(100);
        QCOMPARE(widget.brightness(), 100);
        QCOMPARE(spy.count(), 1);

        widget.setBrightness(-120);
        QCOMPARE(widget.brightness(), -100);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.value(1).value(0).toInt(), -100);
    } {
        QSignalSpy spy(&widget, SIGNAL(contrastChanged(int)));

        widget.setContrast(100);
        QCOMPARE(widget.contrast(), 100);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.value(0).value(0).toInt(), 100);

        widget.setContrast(100);
        QCOMPARE(widget.contrast(), 100);
        QCOMPARE(spy.count(), 1);

        widget.setContrast(-120);
        QCOMPARE(widget.contrast(), -100);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.value(1).value(0).toInt(), -100);
    } {
        QSignalSpy spy(&widget, SIGNAL(hueChanged(int)));

        widget.setHue(100);
        QCOMPARE(widget.hue(), 100);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.value(0).value(0).toInt(), 100);

        widget.setHue(100);
        QCOMPARE(widget.hue(), 100);
        QCOMPARE(spy.count(), 1);

        widget.setHue(-120);
        QCOMPARE(widget.hue(), -100);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.value(1).value(0).toInt(), -100);
    } {
        QSignalSpy spy(&widget, SIGNAL(saturationChanged(int)));

        widget.setSaturation(100);
        QCOMPARE(widget.saturation(), 100);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.value(0).value(0).toInt(), 100);

        widget.setSaturation(100);
        QCOMPARE(widget.saturation(), 100);
        QCOMPARE(spy.count(), 1);

        widget.setSaturation(-120);
        QCOMPARE(widget.saturation(), -100);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.value(1).value(0).toInt(), -100);
    }
}



void tst_QVideoWidget::showWindowControl()
{
    QtTestVideoWidget widget;

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    widget.resize(640, 480);
    QCOMPARE(object.testService->windowControl->displayRect(), QRect(0, 0, 640, 480));

    widget.move(10, 10);
    QCOMPARE(object.testService->windowControl->displayRect(), QRect(0, 0, 640, 480));

    widget.hide();
}

void tst_QVideoWidget::aspectRatioWindowControl()
{
    QtTestVideoObject object(new QtTestWindowControl, nullptr);
    object.testService->windowControl->setAspectRatioMode(Qt::IgnoreAspectRatio);

    QtTestVideoWidget widget;
    object.bind(&widget);

    // Test the aspect ratio defaults to keeping the aspect ratio.
    QCOMPARE(widget.aspectRatioMode(), Qt::KeepAspectRatio);

    // Test the control has been informed of the aspect ratio change, post show.
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.aspectRatioMode(), Qt::KeepAspectRatio);
    QCOMPARE(object.testService->windowControl->aspectRatioMode(), Qt::KeepAspectRatio);

    // Test an aspect ratio change is enforced immediately while visible.
    widget.setAspectRatioMode(Qt::IgnoreAspectRatio);
    QCOMPARE(widget.aspectRatioMode(), Qt::IgnoreAspectRatio);
    QCOMPARE(object.testService->windowControl->aspectRatioMode(), Qt::IgnoreAspectRatio);

    // Test an aspect ratio set while not visible is respected.
    widget.hide();
    widget.setAspectRatioMode(Qt::KeepAspectRatio);
    QCOMPARE(widget.aspectRatioMode(), Qt::KeepAspectRatio);
    widget.show();
    QCOMPARE(widget.aspectRatioMode(), Qt::KeepAspectRatio);
    QCOMPARE(object.testService->windowControl->aspectRatioMode(), Qt::KeepAspectRatio);
}

void tst_QVideoWidget::sizeHintWindowControl_data()
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

    QTest::newRow("800x600, (80,60, 640x480) viewport")
            << QSize(800, 600)
            << QRect(80, 60, 640, 480)
            << QSize(640, 480);
}

void tst_QVideoWidget::sizeHintWindowControl()
{
#ifdef Q_OS_MAC
    QSKIP("QTBUG-26481 - Crashes on Mac");
#endif

    QFETCH(QSize, frameSize);
    QFETCH(QRect, viewport);
    QFETCH(QSize, expectedSize);

    QtTestVideoObject object(nullptr, new QtTestRendererControl);
    QtTestVideoWidget widget;
    object.bind(&widget);

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QVideoSurfaceFormat format(frameSize, QVideoSurfaceFormat::Format_ARGB32);
    format.setViewport(viewport);

    QVERIFY(object.testService->rendererControl->surface()->start(format));

    QCOMPARE(widget.sizeHint(), expectedSize);
}


void tst_QVideoWidget::fullScreenWindowControl()
{
    QtTestVideoObject object(new QtTestWindowControl, nullptr);
    QtTestVideoWidget widget;
    object.bind(&widget);
    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    Qt::WindowFlags windowFlags = widget.windowFlags();

    QSignalSpy spy(&widget, SIGNAL(fullScreenChanged(bool)));

    // Test showing full screen with setFullScreen(true).
    widget.setFullScreen(true);
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(object.testService->windowControl->isFullScreen(), true);
    QCOMPARE(widget.isFullScreen(), true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.value(0).value(0).toBool(), true);

    // Test returning to normal with setFullScreen(false).
    widget.setFullScreen(false);
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(object.testService->windowControl->isFullScreen(), false);
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.value(1).value(0).toBool(), false);
    QCOMPARE(widget.windowFlags(), windowFlags);

    // Test showing full screen with showFullScreen().
    widget.showFullScreen();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(object.testService->windowControl->isFullScreen(), true);
    QCOMPARE(widget.isFullScreen(), true);
    QCOMPARE(spy.count(), 3);
    QCOMPARE(spy.value(2).value(0).toBool(), true);

    // Test returning to normal with showNormal().
    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(object.testService->windowControl->isFullScreen(), false);
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.count(), 4);
    QCOMPARE(spy.value(3).value(0).toBool(), false);
    QCOMPARE(widget.windowFlags(), windowFlags);

    // Test setFullScreen(false) and showNormal() do nothing when isFullScreen() == false.
    widget.setFullScreen(false);
    QCOMPARE(object.testService->windowControl->isFullScreen(), false);
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.count(), 4);
    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(object.testService->windowControl->isFullScreen(), false);
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.count(), 4);

    // Test setFullScreen(true) and showFullScreen() do nothing when isFullScreen() == true.
    widget.showFullScreen();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    widget.setFullScreen(true);
    QCOMPARE(object.testService->windowControl->isFullScreen(), true);
    QCOMPARE(widget.isFullScreen(), true);
    QCOMPARE(spy.count(), 5);
    widget.showFullScreen();
    QCOMPARE(object.testService->windowControl->isFullScreen(), true);
    QCOMPARE(widget.isFullScreen(), true);
    QCOMPARE(spy.count(), 5);

    // Test if the window control exits full screen mode, the widget follows suit.
    object.testService->windowControl->setFullScreen(false);
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.count(), 6);
    QCOMPARE(spy.value(5).value(0).toBool(), false);

    // Test if the window control enters full screen mode, the widget does nothing.
    object.testService->windowControl->setFullScreen(false);
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.count(), 6);
}

void tst_QVideoWidget::color_data()
{
    QTest::addColumn<int>("controlValue");
    QTest::addColumn<int>("value");
    QTest::addColumn<int>("expectedValue");

    QTest::newRow("12")
            << 0
            << 12
            << 12;
    QTest::newRow("-56")
            << 87
            << -56
            << -56;
    QTest::newRow("100")
            << 32
            << 100
            << 100;
    QTest::newRow("1294")
            << 0
            << 1294
            << 100;
    QTest::newRow("-102")
            << 34
            << -102
            << -100;
}

void tst_QVideoWidget::brightnessWindowControl()
{
    QFETCH(int, controlValue);
    QFETCH(int, value);
    QFETCH(int, expectedValue);

    QtTestVideoObject object(new QtTestWindowControl, nullptr);
    object.testService->windowControl->setBrightness(controlValue);

    QtTestVideoWidget widget;
    object.bind(&widget);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    // Test the video widget resets the controls starting brightness to the default.
    QCOMPARE(widget.brightness(), 0);

    QSignalSpy spy(&widget, SIGNAL(brightnessChanged(int)));

    // Test the video widget sets the brightness value, bounded if necessary and emits a changed
    // signal.
    widget.setBrightness(value);
    QCOMPARE(widget.brightness(), expectedValue);
    QCOMPARE(object.testService->windowControl->brightness(), expectedValue);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.value(0).value(0).toInt(), expectedValue);

    // Test the changed signal isn't emitted if the value is unchanged.
    widget.setBrightness(value);
    QCOMPARE(widget.brightness(), expectedValue);
    QCOMPARE(object.testService->windowControl->brightness(), expectedValue);
    QCOMPARE(spy.count(), 1);

    // Test the changed signal is emitted if the brightness is changed internally.
    object.testService->windowControl->setBrightness(controlValue);
    QCOMPARE(widget.brightness(), controlValue);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.value(1).value(0).toInt(), controlValue);
}

void tst_QVideoWidget::brightnessRendererControl()
{
#ifdef Q_OS_MAC
    QSKIP("QTBUG-26481 - Crashes on Mac");
#endif

    QFETCH(int, value);
    QFETCH(int, expectedValue);

    QtTestVideoObject object(nullptr, new QtTestRendererControl);

    QtTestVideoWidget widget;
    object.bind(&widget);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QSignalSpy spy(&widget, SIGNAL(brightnessChanged(int)));

    widget.setBrightness(value);
    QCOMPARE(widget.brightness(), expectedValue);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.value(0).value(0).toInt(), expectedValue);

    widget.setBrightness(value);
    QCOMPARE(widget.brightness(), expectedValue);
    QCOMPARE(spy.count(), 1);
}

void tst_QVideoWidget::contrastWindowControl()
{
    QFETCH(int, controlValue);
    QFETCH(int, value);
    QFETCH(int, expectedValue);

    QtTestVideoObject object(new QtTestWindowControl, nullptr);
    object.testService->windowControl->setContrast(controlValue);

    QtTestVideoWidget widget;
    object.bind(&widget);

    QCOMPARE(widget.contrast(), 0);

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.contrast(), 0);

    QSignalSpy spy(&widget, SIGNAL(contrastChanged(int)));

    widget.setContrast(value);
    QCOMPARE(widget.contrast(), expectedValue);
    QCOMPARE(object.testService->windowControl->contrast(), expectedValue);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.value(0).value(0).toInt(), expectedValue);

    widget.setContrast(value);
    QCOMPARE(widget.contrast(), expectedValue);
    QCOMPARE(object.testService->windowControl->contrast(), expectedValue);
    QCOMPARE(spy.count(), 1);

    object.testService->windowControl->setContrast(controlValue);
    QCOMPARE(widget.contrast(), controlValue);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.value(1).value(0).toInt(), controlValue);
}

void tst_QVideoWidget::contrastRendererControl()
{
#ifdef Q_OS_MAC
    QSKIP("QTBUG-26481 - Crashes on Mac");
#endif

    QFETCH(int, value);
    QFETCH(int, expectedValue);

    QtTestVideoObject object(nullptr, new QtTestRendererControl);

    QtTestVideoWidget widget;
    object.bind(&widget);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QSignalSpy spy(&widget, SIGNAL(contrastChanged(int)));

    widget.setContrast(value);
    QCOMPARE(widget.contrast(), expectedValue);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.value(0).value(0).toInt(), expectedValue);

    widget.setContrast(value);
    QCOMPARE(widget.contrast(), expectedValue);
    QCOMPARE(spy.count(), 1);
}

void tst_QVideoWidget::hueWindowControl()
{
    QFETCH(int, controlValue);
    QFETCH(int, value);
    QFETCH(int, expectedValue);

    QtTestVideoObject object(new QtTestWindowControl, nullptr);
    object.testService->windowControl->setHue(controlValue);

    QtTestVideoWidget widget;
    object.bind(&widget);
    QCOMPARE(widget.hue(), 0);

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.hue(), 0);

    QSignalSpy spy(&widget, SIGNAL(hueChanged(int)));

    widget.setHue(value);
    QCOMPARE(widget.hue(), expectedValue);
    QCOMPARE(object.testService->windowControl->hue(), expectedValue);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.value(0).value(0).toInt(), expectedValue);

    widget.setHue(value);
    QCOMPARE(widget.hue(), expectedValue);
    QCOMPARE(object.testService->windowControl->hue(), expectedValue);
    QCOMPARE(spy.count(), 1);

    object.testService->windowControl->setHue(controlValue);
    QCOMPARE(widget.hue(), controlValue);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.value(1).value(0).toInt(), controlValue);
}

void tst_QVideoWidget::hueRendererControl()
{
#ifdef Q_OS_MAC
    QSKIP("QTBUG-26481 - Crashes on Mac");
#endif

    QFETCH(int, value);
    QFETCH(int, expectedValue);

    QtTestVideoObject object(nullptr, new QtTestRendererControl);

    QtTestVideoWidget widget;
    object.bind(&widget);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QSignalSpy spy(&widget, SIGNAL(hueChanged(int)));

    widget.setHue(value);
    QCOMPARE(widget.hue(), expectedValue);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.value(0).value(0).toInt(), expectedValue);

    widget.setHue(value);
    QCOMPARE(widget.hue(), expectedValue);
    QCOMPARE(spy.count(), 1);
}

void tst_QVideoWidget::saturationWindowControl()
{
    QFETCH(int, controlValue);
    QFETCH(int, value);
    QFETCH(int, expectedValue);

    QtTestVideoObject object(new QtTestWindowControl, nullptr);
    object.testService->windowControl->setSaturation(controlValue);

    QtTestVideoWidget widget;
    object.bind(&widget);
    QCOMPARE(widget.saturation(), 0);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.saturation(), 0);

    QSignalSpy spy(&widget, SIGNAL(saturationChanged(int)));

    widget.setSaturation(value);
    QCOMPARE(widget.saturation(), expectedValue);
    QCOMPARE(object.testService->windowControl->saturation(), expectedValue);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.value(0).value(0).toInt(), expectedValue);

    widget.setSaturation(value);
    QCOMPARE(widget.saturation(), expectedValue);
    QCOMPARE(object.testService->windowControl->saturation(), expectedValue);
    QCOMPARE(spy.count(), 1);

    object.testService->windowControl->setSaturation(controlValue);
    QCOMPARE(widget.saturation(), controlValue);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.value(1).value(0).toInt(), controlValue);
}

void tst_QVideoWidget::saturationRendererControl()
{
#ifdef Q_OS_MAC
    QSKIP("QTBUG-26481 - Crashes on Mac");
#endif

    QFETCH(int, value);
    QFETCH(int, expectedValue);

    QtTestVideoObject object(nullptr, new QtTestRendererControl);

    QtTestVideoWidget widget;
    object.bind(&widget);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QSignalSpy spy(&widget, SIGNAL(saturationChanged(int)));

    widget.setSaturation(value);
    QCOMPARE(widget.saturation(), expectedValue);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.value(0).value(0).toInt(), expectedValue);

    widget.setSaturation(value);
    QCOMPARE(widget.saturation(), expectedValue);
    QCOMPARE(spy.count(), 1);
}

static const uchar rgb32ImageData[] =
{
    0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
    0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00
};

void tst_QVideoWidget::paintRendererControl()
{
    QtTestVideoObject object(nullptr, new QtTestRendererControl);

    QtTestVideoWidget widget;
    object.bind(&widget);
    widget.resize(640,480);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QPainterVideoSurface *surface = qobject_cast<QPainterVideoSurface *>(
            object.testService->rendererControl->surface());

    QVideoSurfaceFormat format(QSize(2, 2), QVideoSurfaceFormat::Format_RGB32);

    QVERIFY(surface->start(format));
    QCOMPARE(surface->isActive(), true);
    QCOMPARE(surface->isReady(), true);

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    QCOMPARE(surface->isActive(), true);
    QCOMPARE(surface->isReady(), true);

    QVideoFrame frame(sizeof(rgb32ImageData), QSize(2, 2), 8, QVideoSurfaceFormat::Format_RGB32);

    frame.map(QVideoFrame::WriteOnly);
    memcpy(frame.bits(), rgb32ImageData, frame.mappedBytes());
    frame.unmap();

    QVERIFY(surface->present(frame));
    QCOMPARE(surface->isActive(), true);
    QCOMPARE(surface->isReady(), false);

    QTRY_COMPARE(surface->isReady(), true);
    QCOMPARE(surface->isActive(), true);
    QCOMPARE(surface->isReady(), true);
}

void tst_QVideoWidget::paintSurface()
{
    QtTestVideoWidget widget;
    widget.resize(640,480);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QVERIFY(widget.videoSurface());
    auto surface = qobject_cast<QPainterVideoSurface *>(
            widget.videoSurface());
    QVERIFY(surface);

    QVideoSurfaceFormat format(QSize(2, 2), QVideoSurfaceFormat::Format_RGB32);
    QVERIFY(surface->start(format));
    QCOMPARE(surface->isActive(), true);

    QVideoFrame frame(sizeof(rgb32ImageData), QSize(2, 2), 8, QVideoSurfaceFormat::Format_RGB32);
    frame.map(QVideoFrame::WriteOnly);
    memcpy(frame.bits(), rgb32ImageData, frame.mappedBytes());
    frame.unmap();

    QVERIFY(surface->present(frame));
    QCOMPARE(surface->isReady(), false);
    QTRY_COMPARE(surface->isReady(), true);
    QCOMPARE(surface->isActive(), true);
    QCOMPARE(surface->isReady(), true);
}

QTEST_MAIN(tst_QVideoWidget)

#include "tst_qvideowidget.moc"
