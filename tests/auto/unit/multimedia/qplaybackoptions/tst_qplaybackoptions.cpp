// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtMultimedia/qplaybackoptions.h>
#include <tuple>

QT_USE_NAMESPACE

class tst_qplaybackoptions : public QObject
{
    Q_OBJECT

private slots:

    void swap_swapsInstances()
    {
        QPlaybackOptions lhs;
        QPlaybackOptions rhs;

        lhs.setNetworkTimeoutMs(1000);
        rhs.setNetworkTimeoutMs(2000);

        swap(lhs, rhs);

        QCOMPARE_EQ(lhs.networkTimeoutMs(), 2000);
        QCOMPARE_EQ(rhs.networkTimeoutMs(), 1000);
    }

    void comparison_comparesLikeTuple_data()
    {
        QTest::addColumn<QPlaybackOptions>("lhs");
        QTest::addColumn<QPlaybackOptions>("rhs");

        QPlaybackOptions lhs;
        QPlaybackOptions rhs;

        const auto intents = { QPlaybackOptions::PlaybackIntent::Playback,
                               QPlaybackOptions::PlaybackIntent::LowLatencyStreaming };

        for (int lhsProbeSize : { 0, 1 }) {
            lhs.setProbeSize(lhsProbeSize);
            for (int rhsProbeSize : { 0, 1 }) {
                rhs.setProbeSize(rhsProbeSize);
                for (QPlaybackOptions::PlaybackIntent lhsIntent : intents) {
                    lhs.setPlaybackIntent(lhsIntent);
                    for (QPlaybackOptions::PlaybackIntent rhsIntent : intents) {
                        rhs.setPlaybackIntent(rhsIntent);
                        for (int lhsTimeout : { 0, 1 }) {
                            lhs.setNetworkTimeoutMs(lhsTimeout);
                            for (int rhsTimeout : { 0, 1 }) {
                                rhs.setNetworkTimeoutMs(rhsTimeout);
                                QTest::newRow(qPrintable(
                                        QString{ "lhs(t=%1,i=%3,p=%5), rhs(t=%2,i=%4,p=%6)" }
                                                .arg(lhsTimeout)
                                                .arg(rhsTimeout)
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

        const auto lhsTuple = std::make_tuple(lhs.networkTimeoutMs(), lhs.playbackIntent(), lhs.probeSize());
        const auto rhsTuple = std::make_tuple(rhs.networkTimeoutMs(), rhs.playbackIntent(), rhs.probeSize());

        QCOMPARE_EQ(lhs == rhs, lhsTuple == rhsTuple);
        QCOMPARE_EQ(lhs != rhs, lhsTuple != rhsTuple);
        QCOMPARE_EQ(lhs < rhs, lhsTuple < rhsTuple);
        QCOMPARE_EQ(lhs <= rhs, lhsTuple <= rhsTuple);
        QCOMPARE_EQ(lhs > rhs, lhsTuple > rhsTuple);
        QCOMPARE_EQ(lhs >= rhs, lhsTuple >= rhsTuple);
    }

    void networkTimeoutMs_returns5seconds_byDefault()
    {
        QPlaybackOptions options;
        QCOMPARE_EQ(options.networkTimeoutMs(), 5'000);
    }

    void setNetworkTimeoutMs_changesTimeout()
    {
        QPlaybackOptions options;

        const int defaultTimeout = options.networkTimeoutMs();
        options.setNetworkTimeoutMs(defaultTimeout + 1);

        QCOMPARE_EQ(options.networkTimeoutMs(), defaultTimeout + 1);
    }

    void resetNetworkTimeoutMs_resetsTimeout()
    {
        QPlaybackOptions options;

        const int defaultTimeout = options.networkTimeoutMs();
        options.setNetworkTimeoutMs(defaultTimeout + 1);
        options.resetNetworkTimeoutMs();

        QCOMPARE_EQ(options.networkTimeoutMs(), defaultTimeout);
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
