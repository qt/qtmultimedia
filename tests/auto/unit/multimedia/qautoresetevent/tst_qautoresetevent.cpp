// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtMultimedia/private/qautoresetevent_p.h>

#include <thread>

// NOLINTBEGIN(readability-convert-member-functions-to-static)

using namespace std::chrono_literals;
QT_USE_NAMESPACE

class tst_QAutoResetEvent : public QObject
{
    Q_OBJECT

    using QAutoResetEvent = QtPrivate::QAutoResetEvent;

private slots:
    void basics();
    void setEventInThread();
    void setEventInThread_multipleTimes();
    void setEventInThread_multipleTimesWithDelay();
};

void tst_QAutoResetEvent::basics()
{
    QAutoResetEvent event;
    QSignalSpy spy{ &event, &QAutoResetEvent::activated };

    QVERIFY(!spy.wait(100ms));

    // one set() call emits signal once
    event.set();
    QVERIFY(spy.wait(1000ms));
    QCOMPARE(spy.count(), 1);

    // multiple set() calls emits signal once
    event.set();
    event.set();
    event.set();
    QVERIFY(spy.wait(1000ms));
    QCOMPARE(spy.count(), 2);
}

void tst_QAutoResetEvent::setEventInThread()
{
    QAutoResetEvent event;
    QSignalSpy spy{ &event, &QAutoResetEvent::activated };

    // one set() call emits signal once
    std::thread setter([&] {
        event.set();
    });

    QVERIFY(spy.wait(1000ms));
    QCOMPARE(spy.count(), 1);
    setter.join();
}

void tst_QAutoResetEvent::setEventInThread_multipleTimes()
{
    QAutoResetEvent event;
    QSignalSpy spy{ &event, &QAutoResetEvent::activated };

    std::atomic<bool> allWritesDone{};

    // multiple set() calls emits signal once
    std::thread setter([&] {
        event.set();
        event.set();
        event.set();
        allWritesDone = true;
    });

    while (!allWritesDone)
        std::this_thread::yield();

    QVERIFY(spy.wait(1000ms));
    QCOMPARE(spy.count(), 1);

    setter.join();
}

void tst_QAutoResetEvent::setEventInThread_multipleTimesWithDelay()
{
    QAutoResetEvent event;
    QSignalSpy spy{ &event, &QAutoResetEvent::activated };

    std::atomic<int> signalsReceived{};

    // multiple set() calls emits signal once
    std::thread setter([&] {
        event.set();
        while (signalsReceived < 1)
            std::this_thread::yield();

        event.set();
        while (signalsReceived < 2)
            std::this_thread::yield();

        event.set();
    });

    QVERIFY(spy.wait(1000ms));
    signalsReceived += 1;

    QVERIFY(spy.wait(1000ms));
    signalsReceived += 1;

    QVERIFY(spy.wait(1000ms));
    QCOMPARE(spy.count(), 3);

    setter.join();
}

QTEST_MAIN(tst_QAutoResetEvent)

#include "tst_qautoresetevent.moc"
