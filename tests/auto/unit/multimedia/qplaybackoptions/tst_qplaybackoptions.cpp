// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtMultimedia/qplaybackoptions.h>
#include <tuple>

using namespace std::chrono_literals;

QT_USE_NAMESPACE

class tst_qplaybackoptions : public QObject
{
    Q_OBJECT

private slots:

    void swap_swapsInstances()
    {
        QPlaybackOptions lhs;
        QPlaybackOptions rhs;

        lhs.setNetworkTimeout(1s);
        rhs.setNetworkTimeout(2s);

        swap(lhs, rhs);

        QCOMPARE_EQ(lhs.networkTimeout(), 2s);
        QCOMPARE_EQ(rhs.networkTimeout(), 1s);
    }

    void comparison_comparesLikeTuple_data()
    {
        QTest::addColumn<QPlaybackOptions>("lhs");
        QTest::addColumn<QPlaybackOptions>("rhs");

        QPlaybackOptions lhs;
        QPlaybackOptions rhs;

        const auto intents = { QPlaybackOptions::PlaybackIntent::Playback,
                               QPlaybackOptions::PlaybackIntent::LowLatencyStreaming };

        for (qsizetype lhsProbeSize : { 0, 1 }) {
            lhs.setProbeSize(lhsProbeSize);
            for (qsizetype rhsProbeSize : { 0, 1 }) {
                rhs.setProbeSize(rhsProbeSize);
                for (QPlaybackOptions::PlaybackIntent lhsIntent : intents) {
                    lhs.setPlaybackIntent(lhsIntent);
                    for (QPlaybackOptions::PlaybackIntent rhsIntent : intents) {
                        rhs.setPlaybackIntent(rhsIntent);
                        for (auto lhsTimeout : { 0ms, 1ms }) {
                            lhs.setNetworkTimeout(lhsTimeout);
                            for (auto rhsTimeout : { 0ms, 1ms }) {
                                rhs.setNetworkTimeout(rhsTimeout);
                                QTest::newRow(qPrintable(
                                        QString{ "lhs(t=%1,i=%3,p=%5), rhs(t=%2,i=%4,p=%6)" }
                                                .arg(lhsTimeout.count())
                                                .arg(rhsTimeout.count())
                                                .arg(qToUnderlying(lhsIntent))
                                                .arg(qToUnderlying(rhsIntent))
                                                .arg(lhsProbeSize)
                                                .arg(rhsProbeSize)))
                                        << lhs << rhs;
                            }
                        }
                    }
                }
            }
        }
    }
    void comparison_comparesLikeTuple()
    {
        QFETCH(QPlaybackOptions, lhs);
        QFETCH(QPlaybackOptions, rhs);

        const auto lhsTuple = std::make_tuple(lhs.networkTimeout(), lhs.playbackIntent(), lhs.probeSize());
        const auto rhsTuple = std::make_tuple(rhs.networkTimeout(), rhs.playbackIntent(), rhs.probeSize());

        QCOMPARE_EQ(lhs == rhs, lhsTuple == rhsTuple);
        QCOMPARE_EQ(lhs != rhs, lhsTuple != rhsTuple);
        QCOMPARE_EQ(lhs < rhs, lhsTuple < rhsTuple);
        QCOMPARE_EQ(lhs <= rhs, lhsTuple <= rhsTuple);
        QCOMPARE_EQ(lhs > rhs, lhsTuple > rhsTuple);
        QCOMPARE_EQ(lhs >= rhs, lhsTuple >= rhsTuple);
    }

    void networkTimeout_returns5seconds_byDefault()
    {
        QPlaybackOptions options;
        QCOMPARE_EQ(options.networkTimeout(), 5s);
    }

    void setNetworkTimeout_changesTimeout()
    {
        QPlaybackOptions options;

        const std::chrono::milliseconds defaultTimeout = options.networkTimeout();
        options.setNetworkTimeout(defaultTimeout + 1ms);

        QCOMPARE_EQ(options.networkTimeout(), defaultTimeout + 1ms);
    }

    void resetNetworkTimeout_resetsTimeout()
    {
        QPlaybackOptions options;

        const std::chrono::milliseconds defaultTimeout = options.networkTimeout();
        options.setNetworkTimeout(defaultTimeout + 1ms);
        options.resetNetworkTimeout();

        QCOMPARE_EQ(options.networkTimeout(), defaultTimeout);
    }
    void playbackIntent_returnsPlayback_byDefault()
    {
        QPlaybackOptions options;
        QCOMPARE_EQ(options.playbackIntent(), QPlaybackOptions::PlaybackIntent::Playback);
    }

    void setPlaybackIntent_changesPlaybackIntent()
    {
        QPlaybackOptions options;

        options.setPlaybackIntent(QPlaybackOptions::PlaybackIntent::LowLatencyStreaming);

        QCOMPARE_EQ(options.playbackIntent(), QPlaybackOptions::PlaybackIntent::LowLatencyStreaming);
    }

    void resetPlaybackIntent_resetsPlaybackIntent()
    {
        QPlaybackOptions options;

        options.setPlaybackIntent(QPlaybackOptions::PlaybackIntent::LowLatencyStreaming);
        options.resetPlaybackIntent();

        QCOMPARE_EQ(options.playbackIntent(), QPlaybackOptions::PlaybackIntent::Playback);
    }

    void probeSize_returnsNegativeOne_byDefault()
    {
        QPlaybackOptions options;
        QCOMPARE_EQ(options.probeSize(), -1);
    }

    void setProbeSize_setsProbeSize()
    {
        QPlaybackOptions options;
        options.setProbeSize(32);
        QCOMPARE_EQ(options.probeSize(),32);
    }

    void resetProbeSize_resetsProbeSize()
    {
        QPlaybackOptions options;
        options.setProbeSize(32);
        options.resetProbeSize();
        QCOMPARE_EQ(options.probeSize(), QPlaybackOptions{}.probeSize());
    }
};

QTEST_MAIN(tst_qplaybackoptions)

#include "tst_qplaybackoptions.moc"
