// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <QDebug>

#include "qmockintegration.h"
#include "qscreencapture.h"
#include "qmocksurfacecapture.h"
#include "qatomic.h"

QT_USE_NAMESPACE

class tst_QScreenCapture : public QObject
{
    Q_OBJECT

private:
    // Use custom waiting instead of QSignalSpy since the spy tries copying not sharable object
    // QVideoFrame to QVariant and gets an assert
    bool waitForFrame(QPlatformSurfaceCapture &psc)
    {
        QAtomicInteger<bool> newFrameReceived = false;
        QObject o;
        auto connection = connect(&psc, &QPlatformSurfaceCapture::newVideoFrame, &o,
                                  [&newFrameReceived]() { newFrameReceived = true; });

        return QTest::qWaitFor([&newFrameReceived]() { return newFrameReceived; });
    }

public slots:
    void initTestCase();
    void init();
    void cleanup();

private slots:
    void destructionOfActiveCapture();

private:
    QMockIntegrationFactory mockIntegrationFactory;
};

void tst_QScreenCapture::initTestCase() { }

void tst_QScreenCapture::init() { }

void tst_QScreenCapture::cleanup() { }

void tst_QScreenCapture::destructionOfActiveCapture()
{
    // Run a few times in order to catch random UB on deletion
    for (int i = 0; i < 10; ++i) {
        auto sc = std::make_unique<QScreenCapture>();
        QPointer<QPlatformSurfaceCapture> psc = QMockIntegration::instance()->lastScreenCapture();
        QVERIFY(psc);

        sc->setActive(true);

        QVERIFY(waitForFrame(*psc));

        QSignalSpy spy(sc.get(), &QScreenCapture::activeChanged);

        psc->setParent(nullptr);
        sc.reset();

        QVERIFY2(spy.empty(), "No signals from QScreenCapture are expected on deletion");
        QVERIFY2(!psc, "Platform screen capture must be deleted whether or not it has a parent");
    }
}

QTEST_MAIN(tst_QScreenCapture)

#include "tst_qscreencapture.moc"
