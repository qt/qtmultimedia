// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtCore/qatomic.h>
#include <QtCore/qstring.h>
#include <QtCore/qthread.h>
#include <QtMultimedia/qsoundeffect.h>
#include <array>
#include <chrono>

using namespace std::chrono_literals;

QT_USE_NAMESPACE

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
