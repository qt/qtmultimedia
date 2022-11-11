// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//TESTED_COMPONENT=plugins/declarative/multimedia

#include <QtTest/QtTest>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QQuickView>
#include <QVideoSink>
#include <QMediaPlayer>

#include "private/qquickvideooutput_p.h"

#include <qobject.h>
#include <qvideoframeformat.h>
#include <qvideoframe.h>

void presentDummyFrame(QVideoSink *sink, const QSize &size)
{
    if (sink) {
        QVideoFrameFormat format(size, QVideoFrameFormat::Format_ARGB8888_Premultiplied);
        QVideoFrame frame(format);

        sink->setVideoFrame(frame);

        // Have to spin an event loop or two for the surfaceFormatChanged() signal
        qApp->processEvents();
    }
}

class tst_QQuickVideoOutput : public QObject
{
    Q_OBJECT
public:
    tst_QQuickVideoOutput();

    ~tst_QQuickVideoOutput() override
    {
        delete m_mappingOutput;
        delete m_mappingComponent;
    }

public slots:
    void initTestCase();

private slots:
    void fillMode();
    void orientation();
    void surfaceSource();
    void paintSurface();
    void sourceRect();

    void contentRect();
    void contentRect_data();

private:
    QQmlEngine m_engine;

    // Variables used for the mapping test
    QQmlComponent *m_mappingComponent = nullptr;
    QQuickVideoOutput *m_mappingOutput = nullptr;

    void updateOutputGeometry(QObject *output);
};

void tst_QQuickVideoOutput::initTestCase()
{
    // We initialize the mapping vars here
    m_mappingComponent = new QQmlComponent(&m_engine);
    m_mappingComponent->loadUrl(QUrl("qrc:/main.qml"));

    auto *component = m_mappingComponent->create();
    QVERIFY(component != nullptr);

    m_mappingOutput = qobject_cast<QQuickVideoOutput *>(component);
    QVERIFY(m_mappingOutput);

    presentDummyFrame(m_mappingOutput->videoSink(), QSize(200,100));
    updateOutputGeometry(m_mappingOutput);
    // First make sure the component has processed the frame
    QCOMPARE(m_mappingOutput->sourceRect(), QRectF(0, 0, 200,100));
}

tst_QQuickVideoOutput::tst_QQuickVideoOutput()
{
}

void tst_QQuickVideoOutput::fillMode()
{
    QQmlComponent component(&m_engine);
    component.loadUrl(QUrl("qrc:/main.qml"));

    QObject *videoOutput = component.create();
    QVERIFY(videoOutput != nullptr);

    QSignalSpy propSpy(videoOutput, SIGNAL(fillModeChanged(QQuickVideoOutput::FillMode)));

    // Default is preserveaspectfit
    QCOMPARE(videoOutput->property("fillMode").value<QQuickVideoOutput::FillMode>(), QQuickVideoOutput::PreserveAspectFit);
    QCOMPARE(propSpy.size(), 0);

    videoOutput->setProperty("fillMode", QVariant(int(QQuickVideoOutput::PreserveAspectCrop)));
    QCOMPARE(videoOutput->property("fillMode").value<QQuickVideoOutput::FillMode>(), QQuickVideoOutput::PreserveAspectCrop);
    QCOMPARE(propSpy.size(), 1);

    videoOutput->setProperty("fillMode", QVariant(int(QQuickVideoOutput::Stretch)));
    QCOMPARE(videoOutput->property("fillMode").value<QQuickVideoOutput::FillMode>(), QQuickVideoOutput::Stretch);
    QCOMPARE(propSpy.size(), 2);

    videoOutput->setProperty("fillMode", QVariant(int(QQuickVideoOutput::Stretch)));
    QCOMPARE(videoOutput->property("fillMode").value<QQuickVideoOutput::FillMode>(), QQuickVideoOutput::Stretch);
    QCOMPARE(propSpy.size(), 2);

    delete videoOutput;
}

void tst_QQuickVideoOutput::orientation()
{
    QQmlComponent component(&m_engine);
    component.loadUrl(QUrl("qrc:/main.qml"));

    QObject *videoOutput = component.create();
    QVERIFY(videoOutput != nullptr);

    QSignalSpy propSpy(videoOutput, SIGNAL(orientationChanged()));

    // Default orientation is 0
    QCOMPARE(videoOutput->property("orientation").toInt(), 0);
    QCOMPARE(propSpy.size(), 0);

    videoOutput->setProperty("orientation", QVariant(90));
    QCOMPARE(videoOutput->property("orientation").toInt(), 90);
    QCOMPARE(propSpy.size(), 1);

    videoOutput->setProperty("orientation", QVariant(180));
    QCOMPARE(videoOutput->property("orientation").toInt(), 180);
    QCOMPARE(propSpy.size(), 2);

    videoOutput->setProperty("orientation", QVariant(270));
    QCOMPARE(videoOutput->property("orientation").toInt(), 270);
    QCOMPARE(propSpy.size(), 3);

    videoOutput->setProperty("orientation", QVariant(360));
    QCOMPARE(videoOutput->property("orientation").toInt(), 360);
    QCOMPARE(propSpy.size(), 4);

    // More than 360 should be fine
    videoOutput->setProperty("orientation", QVariant(540));
    QCOMPARE(videoOutput->property("orientation").toInt(), 540);
    QCOMPARE(propSpy.size(), 5);

    // Negative should be fine
    videoOutput->setProperty("orientation", QVariant(-180));
    QCOMPARE(videoOutput->property("orientation").toInt(), -180);
    QCOMPARE(propSpy.size(), 6);

    // Same value should not reemit
    videoOutput->setProperty("orientation", QVariant(-180));
    QCOMPARE(videoOutput->property("orientation").toInt(), -180);
    QCOMPARE(propSpy.size(), 6);

    // Non multiples of 90 should not work
    videoOutput->setProperty("orientation", QVariant(-1));
    QCOMPARE(videoOutput->property("orientation").toInt(), -180);
    QCOMPARE(propSpy.size(), 6);

    delete videoOutput;
}

void tst_QQuickVideoOutput::surfaceSource()
{
    QQmlComponent component(&m_engine);
    component.loadUrl(QUrl("qrc:/main.qml"));

    QObject *videoOutput = component.create();
    QVERIFY(videoOutput != nullptr);

    QMediaPlayer holder(this);

    QCOMPARE(holder.videoOutput(), nullptr);

    holder.setVideoOutput(videoOutput);

    QVERIFY(holder.videoOutput() != nullptr);
    QVERIFY(holder.videoSink() != nullptr);

    delete videoOutput;

    // This should clear the surface
    QVERIFY(holder.videoOutput() == nullptr);
    QVERIFY(holder.videoSink() == nullptr);

    // Also, creating two sources, setting them in order, and destroying the first
    // should not zero holder.videoSink()
    videoOutput = component.create();
    holder.setVideoOutput(videoOutput);

    QObject *surface = holder.videoOutput();
    QVERIFY(surface != nullptr);

    QObject *videoOutput2 = component.create();
    QVERIFY(videoOutput2);
    holder.setVideoOutput(videoOutput2);
    QVERIFY(holder.videoOutput() != nullptr);
    QVERIFY(holder.videoOutput() != surface); // Surface should have changed
    surface = holder.videoOutput();
    QVERIFY(surface == videoOutput2);

    // Now delete first one
    delete videoOutput;
    QVERIFY(holder.videoOutput() == surface); // Should not have changed surface

    // Now create a second surface and assign it as the source
    // The old surface holder should be zeroed
    QMediaPlayer holder2(this);
    holder2.setVideoOutput(videoOutput2);

    QVERIFY(holder.videoOutput() == nullptr);
    QVERIFY(holder2.videoOutput() != nullptr);

    // Finally a combination - set the same source to two things, then assign a new source
    // to the first output - should not reset the first source
    videoOutput = component.create();
    holder2.setVideoOutput(videoOutput);

    // Both vo and vo2 were pointed to holder2 - setting vo2 should not clear holder2
    QVERIFY(holder2.videoOutput() != nullptr);
    QVERIFY(holder.videoOutput() == nullptr);
    holder.setVideoOutput(videoOutput2);
    QVERIFY(holder2.videoOutput() != nullptr);
    QVERIFY(holder.videoOutput() != nullptr);

    // They should also be independent
    QVERIFY(holder.videoOutput() != holder2.videoOutput());

    delete videoOutput;
    delete videoOutput2;
}

static const uchar rgb32ImageData[] =
{//  B     G     R     A
    0x00, 0x01, 0x02, 0xff, 0x03, 0x04, 0x05, 0xff,
    0x06, 0x07, 0x08, 0xff, 0x09, 0x0a, 0x0b, 0xff,
    0x00, 0x01, 0x02, 0xff, 0x03, 0x04, 0x05, 0xff,
    0x06, 0x07, 0x08, 0xff, 0x09, 0x0a, 0x0b, 0xff,
    0x00, 0x01, 0x02, 0xff, 0x03, 0x04, 0x05, 0xff,
    0x06, 0x07, 0x08, 0xff, 0x09, 0x0a, 0x0b, 0xff,
    0x00, 0x01, 0x02, 0xff, 0x03, 0x04, 0x05, 0xff,
    0x06, 0x07, 0x08, 0xff, 0x09, 0x0a, 0x0b, 0xff
};

void tst_QQuickVideoOutput::paintSurface()
{
    QQuickView window;
    window.setSource(QUrl("qrc:/main.qml"));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto videoOutput = qobject_cast<QQuickVideoOutput *>(window.rootObject());
    QVERIFY(videoOutput);

    auto surface = videoOutput->videoSink();
    QVERIFY(surface);
    videoOutput->setSize(QSize(2, 2));

    QVideoFrame frame(QVideoFrameFormat(QSize(4, 4), QVideoFrameFormat::Format_ARGB8888));
    frame.map(QVideoFrame::ReadWrite);
    QCOMPARE(frame.mappedBytes(0), 64);
    memcpy(frame.bits(0), rgb32ImageData, 64);
    frame.unmap();
    surface->setVideoFrame(frame);
}

void tst_QQuickVideoOutput::sourceRect()
{
    QQmlComponent component(&m_engine);
    component.loadUrl(QUrl("qrc:/main.qml"));

    QObject *videoOutput = component.create();
    QVERIFY(videoOutput != nullptr);

    QMediaPlayer holder(this);

    QSignalSpy propSpy(videoOutput, SIGNAL(sourceRectChanged()));

    holder.setVideoOutput(videoOutput);

    QCOMPARE(videoOutput->property("sourceRect").toRectF(), QRectF());

    presentDummyFrame(holder.videoSink(), QSize(200,100));

    QCOMPARE(videoOutput->property("sourceRect").toRectF(), QRectF(0, 0, 200, 100));
    QCOMPARE(propSpy.size(), 1);

    // Another frame shouldn't cause a source rect change
    presentDummyFrame(holder.videoSink(), QSize(200,100));
    QCOMPARE(propSpy.size(), 1);
    QCOMPARE(videoOutput->property("sourceRect").toRectF(), QRectF(0, 0, 200, 100));

    // Changing orientation and stretch modes should not affect this
    videoOutput->setProperty("orientation", QVariant(90));
    updateOutputGeometry(videoOutput);
    QCOMPARE(videoOutput->property("sourceRect").toRectF(), QRectF(0, 0, 200, 100));

    videoOutput->setProperty("orientation", QVariant(180));
    updateOutputGeometry(videoOutput);
    QCOMPARE(videoOutput->property("sourceRect").toRectF(), QRectF(0, 0, 200, 100));

    videoOutput->setProperty("orientation", QVariant(270));
    updateOutputGeometry(videoOutput);
    QCOMPARE(videoOutput->property("sourceRect").toRectF(), QRectF(0, 0, 200, 100));

    videoOutput->setProperty("orientation", QVariant(-90));
    updateOutputGeometry(videoOutput);
    QCOMPARE(videoOutput->property("sourceRect").toRectF(), QRectF(0, 0, 200, 100));

    videoOutput->setProperty("fillMode", QVariant(int(QQuickVideoOutput::PreserveAspectCrop)));
    updateOutputGeometry(videoOutput);
    QCOMPARE(videoOutput->property("sourceRect").toRectF(), QRectF(0, 0, 200, 100));

    videoOutput->setProperty("fillMode", QVariant(int(QQuickVideoOutput::Stretch)));
    updateOutputGeometry(videoOutput);
    QCOMPARE(videoOutput->property("sourceRect").toRectF(), QRectF(0, 0, 200, 100));

    videoOutput->setProperty("fillMode", QVariant(int(QQuickVideoOutput::Stretch)));
    updateOutputGeometry(videoOutput);
    QCOMPARE(videoOutput->property("sourceRect").toRectF(), QRectF(0, 0, 200, 100));

    delete videoOutput;
}

void tst_QQuickVideoOutput::updateOutputGeometry(QObject *output)
{
    // Since the object isn't visible, update() doesn't do anything
    // so we manually force this
    QMetaObject::invokeMethod(output, "_q_updateGeometry");
}

void tst_QQuickVideoOutput::contentRect()
{
    QFETCH(int, orientation);
    QFETCH(QQuickVideoOutput::FillMode, fillMode);
    QFETCH(QRectF, expected);

    QVERIFY(m_mappingOutput);
    m_mappingOutput->setProperty("orientation", QVariant(orientation));
    m_mappingOutput->setProperty("fillMode", QVariant::fromValue(fillMode));

    updateOutputGeometry(m_mappingOutput);

    QRectF output = m_mappingOutput->property("contentRect").toRectF();
    QCOMPARE(output, expected);
}

void tst_QQuickVideoOutput::contentRect_data()
{
    QTest::addColumn<int>("orientation");
    QTest::addColumn<QQuickVideoOutput::FillMode>("fillMode");
    QTest::addColumn<QRectF>("expected");

    QQuickVideoOutput::FillMode stretch = QQuickVideoOutput::Stretch;
    QQuickVideoOutput::FillMode fit = QQuickVideoOutput::PreserveAspectFit;
    QQuickVideoOutput::FillMode crop = QQuickVideoOutput::PreserveAspectCrop;

    // Stretch just keeps the full render rect regardless of orientation
    QTest::newRow("s0") << 0 << stretch << QRectF(0,0,150,100);
    QTest::newRow("s90") << 90 << stretch << QRectF(0,0,150,100);
    QTest::newRow("s180") << 180 << stretch << QRectF(0,0,150,100);
    QTest::newRow("s270") << 270 << stretch << QRectF(0,0,150,100);

    // Fit depends on orientation
    // Source is 200x100, fitting in 150x100 -> 150x75
    // or 100x200 -> 50x100
    QTest::newRow("f0") << 0 << fit << QRectF(0,12.5f,150,75);
    QTest::newRow("f90") << 90 << fit << QRectF(50,0,50,100);
    QTest::newRow("f180") << 180 << fit << QRectF(0,12.5,150,75);
    QTest::newRow("f270") << 270 << fit << QRectF(50,0,50,100);

    // Crop also depends on orientation, may go outside render rect
    // 200x100 -> -25,0 200x100
    // 100x200 -> 0,-100 150x300
    QTest::newRow("c0") << 0 << crop << QRectF(-25,0,200,100);
    QTest::newRow("c90") << 90 << crop << QRectF(0,-100,150,300);
    QTest::newRow("c180") << 180 << crop << QRectF(-25,0,200,100);
    QTest::newRow("c270") << 270 << crop << QRectF(0,-100,150,300);
}

QTEST_MAIN(tst_QQuickVideoOutput)

#include "tst_qquickvideooutput.moc"
