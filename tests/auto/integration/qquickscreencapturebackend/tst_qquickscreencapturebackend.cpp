// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// TESTED_COMPONENT=src/multimediaquick

#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>

#include <QtCore/qpointer.h>
#include <QtGui/qguiapplication.h>
#include <QtMultimediaQuick/private/qquickscreencapture_p.h>
#include <QtQuick/private/qquickscreen_p.h>

QT_USE_NAMESPACE

class tst_QQuickScreenCaptureBackend : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void ensureQmlScreen_returnsObjectWithNullQScreen_whenQmlScreenIsDefault();
    void ensureQmlScreen_returnsObject_whenPreviousQmlScreenIsDeleted();

    void qmlSetScreen_reusesAssignedQmlScreen_data() { generateTestData(); }
    void qmlSetScreen_reusesAssignedQmlScreen();

    void setScreen_changesQmlScreen_whenInvokedFromCppApi_data() { generateTestData(); }
    void setScreen_changesQmlScreen_whenInvokedFromCppApi();

private:
    void generateTestData();

private:
    QScreen *m_primaryScreen = nullptr;
};

void tst_QQuickScreenCaptureBackend::generateTestData()
{
    QTest::addColumn<QScreen *>("screen");
    QTest::newRow("Null screen") << static_cast<QScreen *>(nullptr);
    QTest::newRow("Primary screen") << m_primaryScreen;
}

void tst_QQuickScreenCaptureBackend::initTestCase()
{
    m_primaryScreen = QGuiApplication::primaryScreen();
    if (!m_primaryScreen)
        QSKIP("Requires a primary screen");

    QScreenCapture capture;
    if (capture.error() == QScreenCapture::CapturingNotSupported)
        QSKIP("Screen capturing not supported");
}

void tst_QQuickScreenCaptureBackend::ensureQmlScreen_returnsObjectWithNullQScreen_whenQmlScreenIsDefault()
{
    // Arrange
    QQuickScreenCatpure capture;

    // Act
    QQuickScreenInfo *qmlScreen = capture.ensureQmlScreen();

    // Assert
    QVERIFY(qmlScreen);
    QVERIFY(!qmlScreen->wrappedScreen());
}

void tst_QQuickScreenCaptureBackend::ensureQmlScreen_returnsObject_whenPreviousQmlScreenIsDeleted()
{
    // Arrange
    QQuickScreenCatpure capture;

    {
        QQuickScreenInfo qmlScreen(nullptr, m_primaryScreen);
        capture.qmlSetScreen(&qmlScreen);
    }

    // Act
    QQuickScreenInfo *qmlScreen = capture.ensureQmlScreen();

    // Assert
    QVERIFY(qmlScreen);
    QCOMPARE(qmlScreen->wrappedScreen(), m_primaryScreen);
}

void tst_QQuickScreenCaptureBackend::qmlSetScreen_reusesAssignedQmlScreen()
{
    // Arrange
    QFETCH(QScreen *, screen);

    QQuickScreenCatpure capture;
    QQuickScreenInfo assignedQmlScreen1(nullptr, screen);
    QQuickScreenInfo assignedQmlScreen2(nullptr, screen);

    capture.qmlSetScreen(&assignedQmlScreen1);
    QQuickScreenInfo *qmlScreen = capture.ensureQmlScreen();

    QSignalSpy screenChangedSpy(&capture, &QQuickScreenCatpure::screenChanged);
    QSignalSpy qmlScreenChangedSpy(&capture, &QQuickScreenCatpure::qmlScreenChanged);

    // Act
    capture.qmlSetScreen(&assignedQmlScreen2);

    // Assert
    QVERIFY(capture.ensureQmlScreen());
    QCOMPARE(capture.ensureQmlScreen(), qmlScreen);
    QCOMPARE(capture.ensureQmlScreen()->wrappedScreen(), screen);

    QCOMPARE(screenChangedSpy.size(), 0);
    QCOMPARE(qmlScreenChangedSpy.size(), 0);
}

void tst_QQuickScreenCaptureBackend::setScreen_changesQmlScreen_whenInvokedFromCppApi()
{
    // Arrange
    QFETCH(QScreen *, screen);

    QQuickScreenCatpure capture;
    capture.setScreen(screen ? nullptr : m_primaryScreen);

    QPointer<QQuickScreenInfo> oldScreenInfo = capture.ensureQmlScreen();
    QSignalSpy qmlScreenChangedSpy(&capture, &QQuickScreenCatpure::qmlScreenChanged);
    QSignalSpy screenChangedSpy(&capture, &QQuickScreenCatpure::screenChanged);

    // Act
    capture.setScreen(screen);

    // Assert
    QVERIFY(oldScreenInfo.isNull());
    QCOMPARE(qmlScreenChangedSpy.size(), 1);
    QCOMPARE(screenChangedSpy.size(), 1);

    QVERIFY(capture.ensureQmlScreen());
    QCOMPARE(qmlScreenChangedSpy.front().front().value<QQuickScreenInfo*>(), capture.ensureQmlScreen());
    QCOMPARE(capture.ensureQmlScreen()->wrappedScreen(), screen);
}

QTEST_MAIN(tst_QQuickScreenCaptureBackend)

#include "tst_qquickscreencapturebackend.moc"
