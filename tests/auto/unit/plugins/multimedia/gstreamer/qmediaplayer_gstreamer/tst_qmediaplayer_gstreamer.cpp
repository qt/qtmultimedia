// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qmediaplayer_gstreamer.h"

#include <QtTest/qtest.h>
#include <QtMultimedia/private/qmediaplayer_p.h>

#include <private/qscopedenvironmentvariable_p.h>

QT_USE_NAMESPACE

using namespace Qt::Literals;

QGStreamerPlatformSpecificInterface *tst_QMediaPlayerGStreamer::gstInterface()
{
    return dynamic_cast<QGStreamerPlatformSpecificInterface *>(
            QPlatformMediaIntegration::instance()->platformSpecificInterface());
}

GstPipeline *tst_QMediaPlayerGStreamer::getGstPipeline()
{
    QGStreamerPlatformSpecificInterface *iface = gstInterface();
    return iface ? iface->gstPipeline(player.get()) : nullptr;
}

QGstPipeline tst_QMediaPlayerGStreamer::getPipeline()
{
    return QGstPipeline{
        getGstPipeline(),
        QGstPipeline::NeedsRef,
    };
}

void tst_QMediaPlayerGStreamer::dumpGraph(const char *fileNamePrefix)
{
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(getGstPipeline()),
                              GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_VERBOSE), fileNamePrefix);
}

tst_QMediaPlayerGStreamer::tst_QMediaPlayerGStreamer()
{
    qputenv("QT_MEDIA_BACKEND", "gstreamer");
}

void tst_QMediaPlayerGStreamer::initTestCase()
{
    using namespace std::chrono_literals;

    QMediaPlayer player;

    QVideoSink sink;
    player.setVideoSink(&sink);
    player.setSource(QUrl("qrc:/testdata/color_matrix.mp4"));

    for (;;) {
        QMediaPlayer::MediaStatus status = player.mediaStatus();
        switch (status) {
        case QMediaPlayer::MediaStatus::InvalidMedia: {
            mediaSupported = false;
            return;
        }
        case QMediaPlayer::MediaStatus::NoMedia:
        case QMediaPlayer::MediaStatus::StalledMedia:
        case QMediaPlayer::MediaStatus::LoadingMedia:
            QTest::qWait(20ms);
            continue;

        default: {
            mediaSupported = true;
            return;
        }
        }
    }
}

void tst_QMediaPlayerGStreamer::init()
{
    player = std::make_unique<QMediaPlayer>();
    mediaStatusSpy.emplace(player.get(), &QMediaPlayer::mediaStatusChanged);
}

void tst_QMediaPlayerGStreamer::cleanup()
{
    player.reset();
}

void tst_QMediaPlayerGStreamer::videoSink_constructor_overridesConversionElement()
{
    if (!mediaSupported)
        QSKIP("Media playback not supported");

    QScopedEnvironmentVariable convOverride{
        "QT_GSTREAMER_OVERRIDE_VIDEO_CONVERSION_ELEMENT",
        "identity name=myConverter",
    };

    QVideoSink sink;
    player->setVideoSink(&sink);
    player->setSource(QUrl("qrc:/testdata/color_matrix.mp4"));
    player->pause();

    QGstPipeline pipeline = getPipeline();
    QTEST_ASSERT(pipeline);
    dumpGraph("videoSink_constructor_overridesConversionElement");

    QTRY_VERIFY(pipeline.findByName("myConverter"));
}

void tst_QMediaPlayerGStreamer::
        videoSink_constructor_overridesConversionElement_withMultipleElements()
{
    if (!mediaSupported)
        QSKIP("Media playback not supported");

    QScopedEnvironmentVariable convOverride{
        "QT_GSTREAMER_OVERRIDE_VIDEO_CONVERSION_ELEMENT",
        "identity name=myConverter ! identity name=myConverter2",
    };

    QVideoSink sink;
    player->setVideoSink(&sink);
    player->setSource(QUrl("qrc:/testdata/color_matrix.mp4"));
    player->pause();

    QGstPipeline pipeline = getPipeline();
    QTEST_ASSERT(pipeline);

    QTRY_VERIFY(pipeline.findByName("myConverter"));
    QTRY_VERIFY(pipeline.findByName("myConverter2"));

    dumpGraph("videoSink_constructer_overridesConversionElement_withMultipleElements");
}

void tst_QMediaPlayerGStreamer::setSource_customGStreamerPipeline_videoTest()
{
    player->setSource(u"gstreamer-pipeline: videotestsrc name=testsrc"_s);

    QGstPipeline pipeline = getPipeline();
    QTEST_ASSERT(pipeline);

    QVERIFY(pipeline.findByName("testsrc"));

    dumpGraph("setSource_customGStreamerPipeline_videoTest");
}

void tst_QMediaPlayerGStreamer::setSource_customGStreamerPipeline_uriDecodeBin()
{
    player->setSource(
            u"gstreamer-pipeline: uridecodebin uri=http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4 name=testsrc"_s);

    QGstPipeline pipeline = getPipeline();
    QTEST_ASSERT(pipeline);

    QVERIFY(pipeline.findByName("testsrc"));

    dumpGraph("setSource_customGStreamerPipeline_uriDecodeBin");
}

QTEST_GUILESS_MAIN(tst_QMediaPlayerGStreamer)

#include "moc_tst_qmediaplayer_gstreamer.cpp"
