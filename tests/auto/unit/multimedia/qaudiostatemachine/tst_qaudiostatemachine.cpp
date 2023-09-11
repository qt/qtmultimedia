// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <private/qaudiostatemachine_p.h>
#include <private/qaudiosystem_p.h>
#include <QThread>

QT_USE_NAMESPACE

template<typename F>
static std::unique_ptr<QThread> createTestThread(std::vector<std::atomic_int> &counters,
                                                 size_t index, F &&functor,
                                                 int minAttemptsCount = 2000)
{
    return std::unique_ptr<QThread>(QThread::create([=, &counters]() {
        auto checkCounter = [=](int counter) { return counter < minAttemptsCount; };
        for (; !QTest::currentTestFailed()
             && std::any_of(counters.begin(), counters.end(), checkCounter);
             ++counters[index])
            functor();
    }));
}

class tst_QAudioStateMachine : public QObject
{
    Q_OBJECT

private slots:
    void constructor_setsStoppedStateWithNoError();

    void start_changesState_whenStateIsStopped_data();
    void start_changesState_whenStateIsStopped();

    void start_doesntChangeState_whenStateIsNotStopped_data();
    void start_doesntChangeState_whenStateIsNotStopped();

    void stop_changesState_whenStateIsNotStopped_data();
    void stop_changesState_whenStateIsNotStopped();

    void stop_doesntChangeState_whenStateIsStopped_data();
    void stop_doesntChangeState_whenStateIsStopped();

    void stopWithDraining_changesState_whenStateIsNotStopped_data();
    void stopWithDraining_changesState_whenStateIsNotStopped();

    void methods_dontChangeState_whenDraining();

    void onDrained_finishesDraining();

    void onDrained_getsFailed_whenDrainHasntBeenCalled_data();
    void onDrained_getsFailed_whenDrainHasntBeenCalled();

    void updateActiveOrIdle_doesntChangeState_whenStateIsNotActiveOrIdle_data();
    void updateActiveOrIdle_doesntChangeState_whenStateIsNotActiveOrIdle();

    void updateActiveOrIdle_changesState_whenStateIsActiveOrIdle_data();
    void updateActiveOrIdle_changesState_whenStateIsActiveOrIdle();

    void suspendAndResume_saveAndRestoreState_whenStateIsActiveOrIdle_data();
    void suspendAndResume_saveAndRestoreState_whenStateIsActiveOrIdle();

    void suspend_doesntChangeState_whenStateIsNotActiveOrIdle_data();
    void suspend_doesntChangeState_whenStateIsNotActiveOrIdle();

    void resume_doesntChangeState_whenStateIsNotSuspended_data();
    void resume_doesntChangeState_whenStateIsNotSuspended();

    void deleteNotifierInSlot_suppressesAdjacentSignal();

    void twoThreadsToggleSuspendResumeAndIdleActive_statesAreConsistent();

    void twoThreadsToggleStartStop_statesAreConsistent();

private:
    void generateNotStoppedPrevStates()
    {
        QTest::addColumn<QAudio::State>("prevState");
        QTest::addColumn<QAudio::Error>("prevError");

        QTest::newRow("from IdleState") << QAudio::IdleState << QAudio::UnderrunError;
        QTest::newRow("from ActiveState") << QAudio::ActiveState << QAudio::NoError;
        QTest::newRow("from SuspendedState") << QAudio::SuspendedState << QAudio::NoError;
    }

    void generateStoppedAndSuspendedPrevStates()
    {
        QTest::addColumn<QAudio::State>("prevState");

        QTest::newRow("from StoppedState") << QAudio::StoppedState;
        QTest::newRow("from SuspendedState") << QAudio::SuspendedState;
    }
};

void tst_QAudioStateMachine::constructor_setsStoppedStateWithNoError()
{
    QAudioStateChangeNotifier changeNotifier;
    QAudioStateMachine stateMachine(changeNotifier);

    QCOMPARE(stateMachine.state(), QAudio::StoppedState);
    QCOMPARE(stateMachine.error(), QAudio::NoError);
    QVERIFY(!stateMachine.isActiveOrIdle());
}

void tst_QAudioStateMachine::start_changesState_whenStateIsStopped_data()
{
    QTest::addColumn<bool>("active");
    QTest::addColumn<QAudio::State>("expectedState");

    QTest::newRow("to active") << true << QAudio::ActiveState;
    QTest::newRow("to not active") << false << QAudio::IdleState;
}

void tst_QAudioStateMachine::start_changesState_whenStateIsStopped()
{
    QFETCH(bool, active);
    QFETCH(QAudio::State, expectedState);

    QAudioStateChangeNotifier changeNotifier;
    QAudioStateMachine stateMachine(changeNotifier);

    QSignalSpy stateSpy(&changeNotifier, &QAudioStateChangeNotifier::stateChanged);
    QSignalSpy errorSpy(&changeNotifier, &QAudioStateChangeNotifier::errorChanged);

    QVERIFY(stateMachine.start(active));

    QCOMPARE(stateSpy.size(), 1);
    QCOMPARE(stateSpy.front().front().value<QAudio::State>(), expectedState);
    QCOMPARE(errorSpy.size(), 0);
    QCOMPARE(stateMachine.state(), expectedState);
    QCOMPARE(stateMachine.error(), QAudio::NoError);
}

void tst_QAudioStateMachine::start_doesntChangeState_whenStateIsNotStopped_data()
{
    generateNotStoppedPrevStates();
}

void tst_QAudioStateMachine::start_doesntChangeState_whenStateIsNotStopped()
{
    QFETCH(QAudio::State, prevState);
    QFETCH(QAudio::Error, prevError);

    QAudioStateChangeNotifier changeNotifier;
    QAudioStateMachine stateMachine(changeNotifier);
    stateMachine.forceSetState(prevState, prevError);

    QSignalSpy stateSpy(&changeNotifier, &QAudioStateChangeNotifier::stateChanged);
    QSignalSpy errorSpy(&changeNotifier, &QAudioStateChangeNotifier::errorChanged);

    QVERIFY2(!stateMachine.start(), "Cannot start (active)");
    QVERIFY2(!stateMachine.start(false), "Cannot start (not active)");

    QCOMPARE(stateSpy.size(), 0);
    QCOMPARE(errorSpy.size(), 0);

    QCOMPARE(stateMachine.state(), prevState);
    QCOMPARE(stateMachine.error(), prevError);
}

void tst_QAudioStateMachine::stop_changesState_whenStateIsNotStopped_data()
{
    generateNotStoppedPrevStates();
}

void tst_QAudioStateMachine::stop_changesState_whenStateIsNotStopped()
{
    QFETCH(QAudio::State, prevState);
    QFETCH(QAudio::Error, prevError);

    QAudioStateChangeNotifier changeNotifier;
    QAudioStateMachine stateMachine(changeNotifier);

    stateMachine.forceSetState(prevState, prevError);

    QSignalSpy stateSpy(&changeNotifier, &QAudioStateChangeNotifier::stateChanged);
    QSignalSpy errorSpy(&changeNotifier, &QAudioStateChangeNotifier::errorChanged);

    auto notifier = stateMachine.stop();
    QVERIFY(notifier);

    QCOMPARE(stateMachine.state(), QAudio::StoppedState);
    QCOMPARE(stateMachine.error(), QAudio::NoError);

    QCOMPARE(stateSpy.size(), 0);
    QCOMPARE(errorSpy.size(), 0);
    QVERIFY(!notifier.isDraining());

    notifier.reset();

    QCOMPARE(stateSpy.size(), 1);
    QCOMPARE(stateSpy.front().front().value<QAudio::State>(), QAudio::StoppedState);
    QCOMPARE(errorSpy.size(), prevError == QAudio::NoError ? 0 : 1);
    if (!errorSpy.empty())
        QCOMPARE(errorSpy.front().front().value<QAudio::Error>(), QAudio::NoError);

    QCOMPARE(stateMachine.state(), QAudio::StoppedState);
    QCOMPARE(stateMachine.error(), QAudio::NoError);
    QVERIFY(!stateMachine.isDraining());
}

void tst_QAudioStateMachine::stop_doesntChangeState_whenStateIsStopped_data()
{
    QTest::addColumn<QAudio::Error>("error");

    QTest::newRow("from NoError") << QAudio::NoError;
    QTest::newRow("from IOError") << QAudio::IOError;
}

void tst_QAudioStateMachine::stop_doesntChangeState_whenStateIsStopped()
{
    QFETCH(QAudio::Error, error);

    QAudioStateChangeNotifier changeNotifier;
    QAudioStateMachine stateMachine(changeNotifier);

    stateMachine.setError(error);

    QSignalSpy stateSpy(&changeNotifier, &QAudioStateChangeNotifier::stateChanged);
    QSignalSpy errorSpy(&changeNotifier, &QAudioStateChangeNotifier::errorChanged);

    QVERIFY2(!stateMachine.stop(), "should return false if already stopped");

    QCOMPARE(stateSpy.size(), 0);
    QCOMPARE(errorSpy.size(), 0);
    QCOMPARE(stateMachine.state(), QAudio::StoppedState);
    QCOMPARE(stateMachine.error(), error);
    QVERIFY(!stateMachine.isDraining());
}

void tst_QAudioStateMachine::stopWithDraining_changesState_whenStateIsNotStopped_data()
{
    generateNotStoppedPrevStates();
}

void tst_QAudioStateMachine::stopWithDraining_changesState_whenStateIsNotStopped()
{
    QFETCH(QAudio::State, prevState);
    QFETCH(QAudio::Error, prevError);

    QAudioStateChangeNotifier changeNotifier;
    QAudioStateMachine stateMachine(changeNotifier);

    stateMachine.forceSetState(prevState, prevError);

    QSignalSpy stateSpy(&changeNotifier, &QAudioStateChangeNotifier::stateChanged);

    auto notifier = stateMachine.stop(QAudio::NoError, true);
    QVERIFY(notifier);
    QCOMPARE(notifier.isDraining(), prevState == QAudio::ActiveState);
    notifier.reset();

    QCOMPARE(stateMachine.state(), QAudio::StoppedState);
    QCOMPARE(stateMachine.error(), QAudio::NoError);
    QCOMPARE(stateMachine.isDraining(), prevState == QAudio::ActiveState);

    QCOMPARE(stateSpy.size(), 1);
}

void tst_QAudioStateMachine::methods_dontChangeState_whenDraining()
{
    QAudioStateChangeNotifier changeNotifier;
    QAudioStateMachine stateMachine(changeNotifier);

    stateMachine.forceSetState(QAudio::ActiveState);
    stateMachine.stop(QAudio::IOError, true);
    QVERIFY(stateMachine.isDraining());

    QSignalSpy stateSpy(&changeNotifier, &QAudioStateChangeNotifier::stateChanged);
    QSignalSpy errorSpy(&changeNotifier, &QAudioStateChangeNotifier::errorChanged);

    QVERIFY(!stateMachine.start());
    QVERIFY(!stateMachine.stop());
    QVERIFY(!stateMachine.stop(QAudio::NoError, true));
    QVERIFY(!stateMachine.suspend());
    QVERIFY(!stateMachine.resume());
    QVERIFY(!stateMachine.updateActiveOrIdle(false));
    QVERIFY(!stateMachine.updateActiveOrIdle(true));

    QCOMPARE(stateMachine.state(), QAudio::StoppedState);
    QCOMPARE(stateMachine.error(), QAudio::IOError);

    QCOMPARE(stateSpy.size(), 0);
    QCOMPARE(errorSpy.size(), 0);

    QVERIFY(stateMachine.isDraining());
}

void tst_QAudioStateMachine::onDrained_finishesDraining()
{
    QAudioStateChangeNotifier changeNotifier;
    QAudioStateMachine stateMachine(changeNotifier);

    stateMachine.forceSetState(QAudio::ActiveState);
    stateMachine.stop(QAudio::IOError, true);
    QVERIFY(stateMachine.isDraining());

    QSignalSpy stateSpy(&changeNotifier, &QAudioStateChangeNotifier::stateChanged);
    QSignalSpy errorSpy(&changeNotifier, &QAudioStateChangeNotifier::errorChanged);

    QVERIFY(stateMachine.onDrained());
    QVERIFY(!stateMachine.isDraining());

    QCOMPARE(stateSpy.size(), 0);
    QCOMPARE(errorSpy.size(), 0);

    QCOMPARE(stateMachine.state(), QAudio::StoppedState);
    QCOMPARE(stateMachine.error(), QAudio::IOError);

    QVERIFY(stateMachine.start());
}

void tst_QAudioStateMachine::onDrained_getsFailed_whenDrainHasntBeenCalled_data()
{
    generateNotStoppedPrevStates();
    QTest::newRow("from Stopped State") << QAudio::StoppedState << QAudio::IOError;
}

void tst_QAudioStateMachine::onDrained_getsFailed_whenDrainHasntBeenCalled()
{
    QFETCH(QAudio::State, prevState);
    QFETCH(QAudio::Error, prevError);

    QAudioStateChangeNotifier changeNotifier;
    QAudioStateMachine stateMachine(changeNotifier);

    stateMachine.forceSetState(prevState, prevError);

    QVERIFY(!stateMachine.onDrained());

    QCOMPARE(stateMachine.state(), prevState);
    QCOMPARE(stateMachine.error(), prevError);
}

void tst_QAudioStateMachine::updateActiveOrIdle_doesntChangeState_whenStateIsNotActiveOrIdle_data()
{
    generateStoppedAndSuspendedPrevStates();
}

void tst_QAudioStateMachine::updateActiveOrIdle_doesntChangeState_whenStateIsNotActiveOrIdle()
{
    QFETCH(QAudio::State, prevState);

    QAudioStateChangeNotifier changeNotifier;
    QAudioStateMachine stateMachine(changeNotifier);

    stateMachine.forceSetState(prevState);

    QSignalSpy stateSpy(&changeNotifier, &QAudioStateChangeNotifier::stateChanged);
    QSignalSpy errorSpy(&changeNotifier, &QAudioStateChangeNotifier::errorChanged);

    QVERIFY(!stateMachine.updateActiveOrIdle(true));
    QVERIFY(!stateMachine.updateActiveOrIdle(false));

    QCOMPARE(stateSpy.size(), 0);
    QCOMPARE(errorSpy.size(), 0);
}

void tst_QAudioStateMachine::updateActiveOrIdle_changesState_whenStateIsActiveOrIdle_data()
{
    QTest::addColumn<QAudio::State>("prevState");
    QTest::addColumn<QAudio::Error>("prevError");
    QTest::addColumn<bool>("active");
    QTest::addColumn<QAudio::Error>("error");

    QTest::newRow("from ActiveState+NoError -> not active+NoError")
            << QAudio::ActiveState << QAudio::NoError << false << QAudio::NoError;
    QTest::newRow("from Idle(UnderrunError) -> active+NoError")
            << QAudio::IdleState << QAudio::UnderrunError << true << QAudio::NoError;
    QTest::newRow("from Idle(UnderrunError) -> not active+UnderrunError")
            << QAudio::IdleState << QAudio::UnderrunError << false << QAudio::UnderrunError;
}

void tst_QAudioStateMachine::updateActiveOrIdle_changesState_whenStateIsActiveOrIdle()
{
    QFETCH(QAudio::State, prevState);
    QFETCH(QAudio::Error, prevError);
    QFETCH(bool, active);
    QFETCH(QAudio::Error, error);

    QAudioStateChangeNotifier changeNotifier;
    QAudioStateMachine stateMachine(changeNotifier);

    stateMachine.forceSetState(prevState, prevError);

    QSignalSpy stateSpy(&changeNotifier, &QAudioStateChangeNotifier::stateChanged);
    QSignalSpy errorSpy(&changeNotifier, &QAudioStateChangeNotifier::errorChanged);

    const auto expectedState = active ? QAudio::ActiveState : QAudio::IdleState;

    auto notifier = stateMachine.updateActiveOrIdle(active, error);
    QVERIFY(notifier);

    QCOMPARE(stateSpy.size(), 0);
    QCOMPARE(errorSpy.size(), 0);

    QCOMPARE(stateMachine.state(), expectedState);
    QCOMPARE(stateMachine.error(), error);

    notifier.reset();

    QCOMPARE(stateSpy.size(), expectedState == prevState ? 0 : 1);
    if (!stateSpy.empty())
        QCOMPARE(stateSpy.front().front().value<QAudio::State>(), expectedState);

    QCOMPARE(errorSpy.size(), prevError == error ? 0 : 1);
    if (!errorSpy.empty())
        QCOMPARE(errorSpy.front().front().value<QAudio::Error>(), error);

    QCOMPARE(stateMachine.state(), expectedState);
    QCOMPARE(stateMachine.error(), error);
}

void tst_QAudioStateMachine::suspendAndResume_saveAndRestoreState_whenStateIsActiveOrIdle_data()
{
    QTest::addColumn<QAudio::State>("prevState");
    QTest::addColumn<QAudio::Error>("prevError");

    QTest::newRow("from Active+NoError") << QAudio::ActiveState << QAudio::NoError;
    QTest::newRow("from Idle+UnderrunError") << QAudio::IdleState << QAudio::UnderrunError;
}

void tst_QAudioStateMachine::suspendAndResume_saveAndRestoreState_whenStateIsActiveOrIdle()
{
    QFETCH(QAudio::State, prevState);
    QFETCH(QAudio::Error, prevError);

    QAudioStateChangeNotifier changeNotifier;
    QAudioStateMachine stateMachine(changeNotifier);

    stateMachine.forceSetState(prevState, prevError);

    QSignalSpy stateSpy(&changeNotifier, &QAudioStateChangeNotifier::stateChanged);
    QSignalSpy errorSpy(&changeNotifier, &QAudioStateChangeNotifier::errorChanged);

    QVERIFY(stateMachine.suspend());

    QCOMPARE(stateSpy.size(), 1);
    QCOMPARE(stateSpy.front().front().value<QAudio::State>(), QAudio::SuspendedState);
    QCOMPARE(errorSpy.size(), prevError == QAudio::NoError ? 0 : 1);

    QCOMPARE(stateMachine.state(), QAudio::SuspendedState);
    QCOMPARE(stateMachine.error(), QAudio::NoError);

    stateSpy.clear();
    errorSpy.clear();

    QVERIFY(!stateMachine.suspend());
    QVERIFY(stateMachine.resume());

    QCOMPARE(stateSpy.size(), 1);
    QCOMPARE(stateSpy.front().front().value<QAudio::State>(), prevState);
    QCOMPARE(errorSpy.size(), 0);

    QCOMPARE(stateMachine.state(), prevState);
    QCOMPARE(stateMachine.error(), QAudio::NoError);
}

void tst_QAudioStateMachine::suspend_doesntChangeState_whenStateIsNotActiveOrIdle_data()
{
    generateStoppedAndSuspendedPrevStates();
}

void tst_QAudioStateMachine::suspend_doesntChangeState_whenStateIsNotActiveOrIdle()
{
    QFETCH(QAudio::State, prevState);

    QAudioStateChangeNotifier changeNotifier;
    QAudioStateMachine stateMachine(changeNotifier);

    stateMachine.forceSetState(prevState);

    QSignalSpy stateSpy(&changeNotifier, &QAudioStateChangeNotifier::stateChanged);
    QSignalSpy errorSpy(&changeNotifier, &QAudioStateChangeNotifier::errorChanged);

    QVERIFY(!stateMachine.suspend());

    QCOMPARE(stateSpy.size(), 0);
    QCOMPARE(errorSpy.size(), 0);

    QCOMPARE(stateMachine.state(), prevState);
    QCOMPARE(stateMachine.error(), QAudio::NoError);
}

void tst_QAudioStateMachine::resume_doesntChangeState_whenStateIsNotSuspended_data()
{
    QTest::addColumn<QAudio::State>("prevState");

    QTest::newRow("from StoppedState") << QAudio::StoppedState;
    QTest::newRow("from ActiveState") << QAudio::ActiveState;
    QTest::newRow("from IdleState") << QAudio::IdleState;
}

void tst_QAudioStateMachine::resume_doesntChangeState_whenStateIsNotSuspended()
{
    QFETCH(QAudio::State, prevState);

    QAudioStateChangeNotifier changeNotifier;
    QAudioStateMachine stateMachine(changeNotifier);

    stateMachine.forceSetState(prevState);

    QSignalSpy stateSpy(&changeNotifier, &QAudioStateChangeNotifier::stateChanged);
    QSignalSpy errorSpy(&changeNotifier, &QAudioStateChangeNotifier::errorChanged);

    QVERIFY(!stateMachine.resume());

    QCOMPARE(stateSpy.size(), 0);
    QCOMPARE(errorSpy.size(), 0);

    QCOMPARE(stateMachine.state(), prevState);
    QCOMPARE(stateMachine.error(), QAudio::NoError);
}

void tst_QAudioStateMachine::deleteNotifierInSlot_suppressesAdjacentSignal()
{
    auto changeNotifier = std::make_unique<QAudioStateChangeNotifier>();
    QAudioStateMachine stateMachine(*changeNotifier);
    stateMachine.start();

    auto onSignal = [&]() {
        QVERIFY2(changeNotifier, "The 2nd signal shouldn't be emitted");
        changeNotifier.reset();
    };

    connect(changeNotifier.get(), &QAudioStateChangeNotifier::errorChanged,
            this, onSignal, Qt::DirectConnection);
    connect(changeNotifier.get(), &QAudioStateChangeNotifier::stateChanged,
            this, onSignal, Qt::DirectConnection);

    stateMachine.stop(QAudio::IOError);
}

void tst_QAudioStateMachine::twoThreadsToggleSuspendResumeAndIdleActive_statesAreConsistent()
{
    QAudioStateChangeNotifier changeNotifier;
    QAudioStateMachine stateMachine(changeNotifier);

    QVERIFY(stateMachine.start());
    QCOMPARE(stateMachine.state(), QAudio::ActiveState);

    std::atomic<int> signalsCount = 0;
    std::atomic<int> changesCount = 0;

    connect(&changeNotifier, &QAudioStateChangeNotifier::stateChanged,
            this, [&](QAudio::State) { ++signalsCount; }, Qt::DirectConnection);

    std::vector<std::atomic_int> counters(2);

    auto threadSuspendResume = createTestThread(counters, 0, [&]() {
        {
            auto notifier = stateMachine.suspend();
            QVERIFY(notifier);
            QVERIFY(notifier.isStateChanged());
            QCOMPARE(notifier.audioState(), QAudio::SuspendedState);
            ++changesCount;
        }

        {
            auto notifier = stateMachine.resume();
            QVERIFY(notifier);
            QVERIFY(notifier.isStateChanged());
            QCOMPARE_NE(notifier.audioState(), QAudio::SuspendedState);
            ++changesCount;
        }
    });

    auto threadIdleActive = createTestThread(counters, 1, [&]() {
        if (auto notifier = stateMachine.updateActiveOrIdle(false)) {
            if (notifier.isStateChanged())
                ++changesCount;

            QCOMPARE(notifier.audioState(), QAudio::IdleState);
        }

        if (auto notifier = stateMachine.updateActiveOrIdle(true)) {
            if (notifier.isStateChanged())
                ++changesCount;

            QCOMPARE(notifier.audioState(), QAudio::ActiveState);
        }
    });

    threadSuspendResume->start();
    threadIdleActive->start();

    threadSuspendResume->wait();
    threadIdleActive->wait();

    if (QTest::currentTestFailed()) {
        qDebug() << "counterSuspendResume:" << counters[0];
        qDebug() << "counterIdleActive:" << counters[1];
    }

    QCOMPARE(signalsCount, changesCount);
}

void tst_QAudioStateMachine::twoThreadsToggleStartStop_statesAreConsistent()
{
    QAudioStateChangeNotifier changeNotifier;
    QAudioStateMachine stateMachine(changeNotifier);

    QVERIFY(stateMachine.start());
    QCOMPARE(stateMachine.state(), QAudio::ActiveState);

    std::atomic<int> signalsCount = 0;
    std::atomic<int> changesCount = 0;

    connect(&changeNotifier, &QAudioStateChangeNotifier::stateChanged,
            this, [&](QAudio::State) { ++signalsCount; }, Qt::DirectConnection);

    std::vector<std::atomic_int> counters(2);

    auto threadStartActive = createTestThread(counters, 0, [&]() {
        if (auto startNotifier = stateMachine.start()) {
            QCOMPARE(startNotifier.prevAudioState(), QAudio::StoppedState);
            QCOMPARE(startNotifier.audioState(), QAudio::ActiveState);
            ++changesCount;
            startNotifier.reset();

            auto stopNotifier = stateMachine.stop();
            ++changesCount;
            QVERIFY(stopNotifier);
            QCOMPARE(stopNotifier.prevAudioState(), QAudio::ActiveState);
        }
    });

    auto threadStartIdle = createTestThread(counters, 1, [&]() {
        if (auto startNotifier = stateMachine.start(false)) {
            QCOMPARE(startNotifier.prevAudioState(), QAudio::StoppedState);
            QCOMPARE(startNotifier.audioState(), QAudio::IdleState);
            ++changesCount;
            startNotifier.reset();

            auto stopNotifier = stateMachine.stop();
            ++changesCount;
            QVERIFY(stopNotifier);
            QCOMPARE(stopNotifier.audioState(), QAudio::StoppedState);
            QCOMPARE(stopNotifier.prevAudioState(), QAudio::IdleState);
        }
    });

    threadStartActive->start();
    threadStartIdle->start();

    threadStartActive->wait();
    threadStartIdle->wait();

    if (QTest::currentTestFailed()) {
        qDebug() << "counterSuspendResume:" << counters[0];
        qDebug() << "counterIdleActive:" << counters[1];
    }

    QCOMPARE(signalsCount, changesCount);
}

QTEST_GUILESS_MAIN(tst_QAudioStateMachine)

#include "tst_qaudiostatemachine.moc"
