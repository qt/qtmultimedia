// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtCore/qobject.h>
#include <QtCore/qdebug.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
}

class tst_ffmpeg_utils : public QObject
{
    Q_OBJECT

private slots:
    void codecCapabilities_toString()
    {
        QFFmpeg::AVCodecCapabilities caps{ AV_CODEC_CAP_DELAY | AV_CODEC_CAP_HARDWARE };
        QString result = QFFmpeg::toString(caps);
        QVERIFY(result.contains("DELAY"));
        QVERIFY(result.contains("HARDWARE"));
        QVERIFY(result.contains(", "));
    }

    void codecCapabilities_toDebugString()
    {
        QFFmpeg::AVCodecCapabilities caps{ AV_CODEC_CAP_FRAME_THREADS };
        QString debugOutput;
        QDebug dbg(&debugOutput);
        dbg << caps;
        QVERIFY(debugOutput.contains("FRAME_THREADS"));
    }

    void codecCapabilities_empty()
    {
        QFFmpeg::AVCodecCapabilities caps{ 0 };
        QString result = QFFmpeg::toString(caps);
        QCOMPARE(result, QString());
    }

    void pixelFormatFlags_toString()
    {
        QFFmpeg::AVPixelFormatFlags flags{ AV_PIX_FMT_FLAG_PLANAR | AV_PIX_FMT_FLAG_RGB };
        QString result = QFFmpeg::toString(flags);
        QVERIFY(result.contains("PLANAR"));
        QVERIFY(result.contains("RGB"));
        QVERIFY(result.contains(", "));
    }

    void pixelFormatFlags_toDebugString()
    {
        QFFmpeg::AVPixelFormatFlags flags{ AV_PIX_FMT_FLAG_HWACCEL };
        QString debugOutput;
        QDebug dbg(&debugOutput);
        dbg << flags;
        QVERIFY(debugOutput.contains("HWACCEL"));
    }

    void pixelFormatFlags_empty()
    {
        QFFmpeg::AVPixelFormatFlags flags{ 0 };
        QString result = QFFmpeg::toString(flags);
        QCOMPARE(result, QString());
    }

    void hwConfigMethods_toString()
    {
        QFFmpeg::AVHwConfigMethods methods{ AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX
                                            | AV_CODEC_HW_CONFIG_METHOD_INTERNAL };
        QString result = QFFmpeg::toString(methods);
        QVERIFY(result.contains("HW_DEVICE_CTX"));
        QVERIFY(result.contains("INTERNAL"));
        QVERIFY(result.contains(", "));
    }

    void hwConfigMethods_toDebugString()
    {
        QFFmpeg::AVHwConfigMethods methods{ AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX };
        QString debugOutput;
        QDebug dbg(&debugOutput);
        dbg << methods;
        QVERIFY(debugOutput.contains("HW_FRAMES_CTX"));
    }

    void hwConfigMethods_empty()
    {
        QFFmpeg::AVHwConfigMethods methods{ 0 };
        QString result = QFFmpeg::toString(methods);
        QCOMPARE(result, QString());
    }

    void codecCapabilities_roundtrip()
    {
        const int allFlags = AV_CODEC_CAP_DRAW_HORIZ_BAND | AV_CODEC_CAP_DR1 |
                             AV_CODEC_CAP_DELAY | AV_CODEC_CAP_SMALL_LAST_FRAME |
                             AV_CODEC_CAP_EXPERIMENTAL | AV_CODEC_CAP_CHANNEL_CONF |
                             AV_CODEC_CAP_FRAME_THREADS | AV_CODEC_CAP_SLICE_THREADS;
        QFFmpeg::AVCodecCapabilities caps{ allFlags };
        QString result = QFFmpeg::toString(caps);
        QVERIFY(!result.isEmpty());
        QVERIFY(result.contains("DRAW_HORIZ_BAND"));
        QVERIFY(result.contains("EXPERIMENTAL"));
    }
};

QTEST_GUILESS_MAIN(tst_ffmpeg_utils)

#include "tst_ffmpeg_utils.moc"
