// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimediaTestLib/private/mediabackendutils_p.h>
#include <QtTest/qtest.h>
#include <QtMultimedia/qsoundeffect.h>
#include <QtMultimedia/private/qsamplecache_p.h>
#include <QtMultimedia/private/qsoundeffectwithplayer_p.h>
#include <QtCore/qatomic.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qstring.h>
#include <QtCore/qthread.h>

#include <array>
#include <chrono>

using namespace std::chrono_literals;


using namespace Qt::StringLiterals;

class tst_qsoundeffect_concurrent : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    // Verify that QSoundEffect plays sound and does not cause asserts when
    // QSoundEffect is constructed on worker thread before media backend is initialized.
    // See also QTBUG-129597
    void play_playsSound_whenMediaBackendInitializedOnWorkerThread()
    {
        const QUrl url{ "qrc:/double-drop.wav"_L1 };

        QAtomicInteger success = true;
        constexpr size_t threadCount = 3;
        std::array<std::unique_ptr<QObject>, threadCount> contextElements;
        std::array<std::unique_ptr<QThread>, threadCount> threads;

        // Stress test QSoundEffect a bit to make sure it works concurrently
        // in multiple threads
        for (size_t i = 0; i < threadCount; ++i) {
            threads[i] = std::make_unique<QThread>();
            threads[i]->start();

            auto contextElement = new QObject{};
            contextElements[i].reset(contextElement);
            contextElement->moveToThread(threads[i].get());
            QMetaObject::invokeMethod(contextElement, [&, thread = threads[i].get()] {
                if (!playSound(url, thread))
                    success = false;
                thread->exit();
            });
        }

        for (size_t i = 0; i < threadCount; ++i)
            QTRY_VERIFY(threads[i]->wait(10ms));

        QVERIFY(success);

        // Bonus test, make sure playing sound works from main thread also
        QVERIFY(playSound(url));
    }

    // the worker keeps its event loop alive while the voice event is dropped
    // on the engine thread (restart) and on the worker (natural finish)
    void restartPlayback_inWorkerThread_doesNotWarn()
    {
        QTest::failOnWarning(QRegularExpression(u".*(another thread|Invalid socket).*"_s));

        const QUrl url{ "qrc:/double-drop.wav"_L1 };

        QThread worker;
        worker.start();

        QObject context;
        context.moveToThread(&worker);

        std::atomic<bool> finished{};
        std::atomic<bool> effectReady{};
        QMetaObject::invokeMethod(&context, [&] {
            auto waitUntil = [](auto &&predicate, std::chrono::milliseconds timeout) {
                auto start = std::chrono::steady_clock::now();
                while (!predicate() && std::chrono::steady_clock::now() - start < timeout)
                    QThread::currentThread()->eventDispatcher()->processEvents(
                            QEventLoop::AllEvents);
            };

            auto effect = std::make_unique<QSoundEffect>();
            effect->setSource(url);
            effect->setLoopCount(QSoundEffect::Infinite);
            effect->play();

            waitUntil([&] {
                return effect->status() != QSoundEffect::Loading;
            }, 60s);
            if (effect->status() == QSoundEffect::Ready) {
                effectReady = true;

                // restart while the infinite loop is playing: the previous voice is dropped
                effect->play();
                effect->stop();
                effect->play();
                waitUntil([] {
                    return false;
                }, 200ms);

                // stop: the voice is released by the engine thread
                effect->stop();
                waitUntil([] {
                    return false;
                }, 100ms);

                // natural finish: the voice is released by the worker
                effect->setLoopCount(1);
                effect->play();
                waitUntil([&] {
                    return !effect->isPlaying();
                }, 30s);
            }

            effect.reset();
            finished = true;
        }, Qt::QueuedConnection);

        const bool done = QTest::qWaitFor([&] {
            return finished.load();
        }, 120s);

        worker.quit();
        QVERIFY(worker.wait(5000));

        QVERIFY(done);
        if (!effectReady)
            QSKIP("No audio output available; the reproducer needs a sound device");
    }

    // the voice's loop event moves to the engine thread before the voice is
    // released there, so dropping the last reference must not touch a notifier of another thread
    void voice_releasedOnOtherThread_doesNotWarn()
    {
        QTest::failOnWarning(QRegularExpression(u".*(another thread|Invalid socket).*"_s));

        QAudioFormat format;
        format.setSampleFormat(QAudioFormat::Float);
        format.setChannelCount(1);
        format.setSampleRate(44100);
        const std::array<float, 2> sampleData{ 1.0f, 0.5f };
        auto sample = std::make_shared<QSample>(
                QByteArray(reinterpret_cast<const char *>(sampleData.data()),
                           sizeof(sampleData)),
                format);

        QThread worker;
        worker.start();

        QObject context;
        context.moveToThread(&worker);

        std::shared_ptr<QtMultimediaPrivate::QSoundEffectVoice> voice;
        QMetaObject::invokeMethod(&context, [&] {
            voice = std::make_shared<QtMultimediaPrivate::QSoundEffectVoice>(
                    QtMultimediaPrivate::VoiceId{ 0 }, sample, 1.0f, false, 1, format);
            voice->m_currentLoopChanged.moveToThread(QCoreApplication::instance()->thread());
        }, Qt::BlockingQueuedConnection);
        QVERIFY(voice);

        voice.reset();

        QMetaObject::invokeMethod(&context, [&] {
            worker.quit();
        }, Qt::BlockingQueuedConnection);
        QVERIFY(worker.wait(5000));
    }

private:
    bool playSound(const QUrl &url, QThread *currentThread = nullptr)
    {
        QSoundEffect effect;
        effect.setSource(url);
        effect.setLoopCount(5);
        effect.play();

        auto waitCondition = [&] {
            // Error is success because CI might not have audio device.
            // We still want to run this test in CI to uncover any crashes or asserts
            return effect.isPlaying() || effect.status() == QSoundEffect::Error;
        };

        if (currentThread) {
            auto start = std::chrono::steady_clock::now();
            while (std::chrono::steady_clock::now() - start < 60s) {
                if (waitCondition())
                    return true;
                currentThread->eventDispatcher()->processEvents(QEventLoop::AllEvents);
            };
            return false;
        } else {
            return QTest::qWaitFor([&] {
                return waitCondition();
            }, 60s);
        }
    }
};

// Don't initialize Gui because we want to test the situation
// where COM is not already initialized on the main thread.
QTEST_GUILESS_MAIN(tst_qsoundeffect_concurrent);
#include "tst_qsoundeffect_concurrent.moc"
