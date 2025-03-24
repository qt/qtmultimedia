// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtCore/qthread.h>
#include <QtMultimedia/qmediaplayer.h>
#include <QtMultimedia/qaudiooutput.h>
#include <private/testvideosink_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qatomic.h>
#include <chrono>

using namespace std::chrono_literals;

QT_USE_NAMESPACE

using namespace Qt::StringLiterals;

class tst_qmediaplayer_concurrent : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    // Verify that QMediaPlayer plays sound and does not cause asserts when
    // QMediaPlayer is constructed on worker thread before media backend is initialized.
    // See also QTBUG-129597
    void play_playsVideo_whenMediaBackendInitializedOnWorkerThread()
    {
        const QUrl url{ "qrc:3colors_with_sound_1s.mp4"_L1 };

        size_t threadCount = 3;
        QAtomicInteger success = true;
        std::vector<std::unique_ptr<QThread>> threads(threadCount);

        // Stress test QMediaPlayer a bit to make sure it works concurrently
        // in multiple threads
        for (size_t i = 0; i < threadCount; ++i) {
            threads[i].reset(QThread::create([&] {
                if (!playVideo(url))
                    success = false;
            }));

            threads[i]->start();
        }

        for (size_t i = 0; i < threadCount; ++i)
            QVERIFY(threads[i]->wait());

        QVERIFY(success);
    }

private:
    bool playVideo(const QUrl &url)
    {
        QAudioOutput output;
        TestVideoSink surface;
        QMediaPlayer player;
        player.setAudioOutput(&output);
        player.setVideoSink(&surface);
        player.setSource(url);
        player.play();

        bool stopped = false;
        connect(&player, &QMediaPlayer::playbackStateChanged, &player,
                [&](QMediaPlayer::PlaybackState state) {
                    if (state == QMediaPlayer::StoppedState)
                        stopped = true;
                });

        return QTest::qWaitFor(
                [&] { //
                    return stopped;
                },
                60s) && player.error() == QMediaPlayer::NoError;
    }
};

// Don't initialize Gui because we want to test the situation
// where COM is not already initialized on the main thread.
QTEST_GUILESS_MAIN(tst_qmediaplayer_concurrent);
#include "tst_qmediaplayer_concurrent.moc"
