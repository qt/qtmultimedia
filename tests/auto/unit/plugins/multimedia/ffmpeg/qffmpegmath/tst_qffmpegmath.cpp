// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtCore/qobject.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>

extern "C" {
#include "libavutil/mathematics.h"
}

QT_USE_NAMESPACE

class tst_qffmpegmath : public QObject
{
    Q_OBJECT

private slots:
    void mul_agreesWith_av_rescale_withFiniteNumbers() {
        for (qint64 number = -20; number < 30; ++number) {
            const std::optional<qint64> actual = QFFmpeg::mul(number, AVRational{ 1, 10 });
            QVERIFY(actual);
            const qint64 expected = av_rescale(number, 1, 10);
            QCOMPARE_EQ(actual, expected);
        }
    }
};

QTEST_GUILESS_MAIN(tst_qffmpegmath)

#include "tst_qffmpegmath.moc"
