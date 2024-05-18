// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_gstreamer_backend.h"

#include <QtTest/QtTest>

#include <QtQGstreamerMediaPlugin/private/qgst_handle_types_p.h>
#include <QtQGstreamerMediaPlugin/private/qgst_p.h>
#include <QtQGstreamerMediaPlugin/private/qgstpipeline_p.h>
#include <QtQGstreamerMediaPlugin/private/qgstreamermetadata_p.h>

QT_USE_NAMESPACE

// NOLINTBEGIN(readability-convert-member-functions-to-static)

using namespace Qt::Literals;

QGstTagListHandle tst_GStreamer::parseTagList(const char *str)
{
    QGstTagListHandle tagList{
        gst_tag_list_new_from_string(str),
        QGstTagListHandle::NeedsRef,
    };
    return tagList;
}

QGstTagListHandle tst_GStreamer::parseTagList(const QByteArray &ba)
{
    return parseTagList(ba.constData());
}

void tst_GStreamer::qGstCasts_withElement()
{
    QGstElement element = QGstElement::createFromFactory("identity", "myPipeline");
    QVERIFY(element);

    QVERIFY(!qIsGstObjectOfType<GstPipeline>(element.element()));
    QVERIFY(!qIsGstObjectOfType<GstBin>(element.element()));
}

void tst_GStreamer::qGstCasts_withBin()
{
    QGstBin bin = QGstBin::create("bin");
    QVERIFY(bin);

    QVERIFY(!qIsGstObjectOfType<GstPipeline>(bin.element()));
    QVERIFY(qIsGstObjectOfType<GstBin>(bin.element()));
}

void tst_GStreamer::qGstCasts_withPipeline()
{
    QGstPipeline pipeline = QGstPipeline::create("myPipeline");

    QGstElement element{
        qGstSafeCast<GstElement>(pipeline.pipeline()),
        QGstElement::NeedsRef,
    };

    QVERIFY(element);
    QVERIFY(qIsGstObjectOfType<GstPipeline>(element.element()));
    QVERIFY(qIsGstObjectOfType<GstBin>(element.element()));
}

void tst_GStreamer::metadata_taglistToMetaData()
{
    QGstTagListHandle tagList = parseTagList(R"(taglist, title="My Video", comment="yada")");

    QMediaMetaData parsed = taglistToMetaData(tagList);

    QCOMPARE(parsed.stringValue(QMediaMetaData::Title), u"My Video"_s);
    QCOMPARE(parsed.stringValue(QMediaMetaData::Comment), u"yada"_s);
}

void tst_GStreamer::metadata_taglistToMetaData_extractsOrientation()
{
    QFETCH(QByteArray, taglist);
    QFETCH(QtVideo::Rotation, rotation);

    QGstTagListHandle tagList = parseTagList(taglist);
    QMediaMetaData parsed = taglistToMetaData(tagList);
    QCOMPARE(parsed[QMediaMetaData::Orientation].value<QtVideo::Rotation>(), rotation);
}

void tst_GStreamer::metadata_taglistToMetaData_extractsOrientation_data()
{
    QTest::addColumn<QByteArray>("taglist");
    QTest::addColumn<QtVideo::Rotation>("rotation");

    QTest::newRow("no rotation") << R"(taglist, title="My Video", comment="yada")"_ba
                                 << QtVideo::Rotation::None;
    QTest::newRow("90 degree")
            << R"(taglist, title="My Video", comment="yada", image-orientation=(string)rotate-90)"_ba
            << QtVideo::Rotation::Clockwise90;
    QTest::newRow("180 degree")
            << R"(taglist, title="My Video", comment="yada", image-orientation=(string)rotate-180)"_ba
            << QtVideo::Rotation::Clockwise180;
    QTest::newRow("270 degree")
            << R"(taglist, title="My Video", comment="yada", image-orientation=(string)rotate-270)"_ba
            << QtVideo::Rotation::Clockwise270;
}

void tst_GStreamer::metadata_taglistToMetaData_extractsDuration()
{
    QGstTagListHandle tagList = parseTagList(
            R"__(taglist, video-codec=(string)"On2\ VP9",  container-specific-track-id=(string)1, extended-comment=(string){ "ALPHA_MODE\=1", "HANDLER_NAME\=Apple\ Video\ Media\ Handler", "VENDOR_ID\=appl", "TIMECODE\=00:00:00:00", "DURATION\=00:00:00.400000000" }, encoder=(string)"Lavc59.37.100\ libvpx-vp9")__");

    QMediaMetaData parsed = taglistToMetaData(tagList.get());

    QCOMPARE(parsed[QMediaMetaData::Duration].value<int>(), 400);
}

void tst_GStreamer::QGstBin_createFromPipelineDescription()
{
    QGstBin bin = QGstBin::createFromPipelineDescription("identity name=foo ! identity name=bar");

    QVERIFY(bin);
    QVERIFY(bin.findByName("foo"));
    QCOMPARE_EQ(bin.findByName("foo").getParent(), bin);
    QVERIFY(bin.findByName("bar"));
    QVERIFY(!bin.findByName("baz"));
    bin.dumpGraph("QGstBin_createFromPipelineDescription");
}

void tst_GStreamer::QGstElement_createFromPipelineDescription()
{
    using namespace std::string_view_literals;
    QGstElement element = QGstElement::createFromPipelineDescription("identity name=foo");
    QCOMPARE_EQ(element.name().constData(), "foo"sv);
    QCOMPARE_EQ(element.typeName().constData(), "GstIdentity"sv);
}

void tst_GStreamer::QGstElement_createFromPipelineDescription_multipleElementsCreatesBin()
{
    using namespace std::string_view_literals;
    QGstElement element =
            QGstElement::createFromPipelineDescription("identity name=foo ! identity name=bar");

    QVERIFY(element);
    QCOMPARE_EQ(element.typeName().constData(), "GstPipeline"sv);

    QGstBin bin{
        qGstSafeCast<GstBin>(element.element()),
        QGstBin::NeedsRef,
    };

    QVERIFY(bin);
    QVERIFY(bin.findByName("foo"));
    QCOMPARE_EQ(bin.findByName("foo").getParent(), bin);
    QVERIFY(bin.findByName("bar"));
    QVERIFY(!bin.findByName("baz"));

    bin.dumpGraph("QGstElement_createFromPipelineDescription_multipleElements");
}

void tst_GStreamer::QGstPad_inferTypeFromName()
{
    auto makePad = [](const char *name, GstPadDirection direction) {
        return QGstPad{
            gst_pad_new(name, direction),
            QGstPad::NeedsRef,
        };
    };

    QVERIFY(makePad("audio_0", GST_PAD_SRC).inferTrackTypeFromName()
            == QPlatformMediaPlayer::AudioStream);
    QVERIFY(makePad("video_0", GST_PAD_SRC).inferTrackTypeFromName()
            == QPlatformMediaPlayer::VideoStream);
    QVERIFY(makePad("text_0", GST_PAD_SRC).inferTrackTypeFromName()
            == QPlatformMediaPlayer::SubtitleStream);
    QVERIFY(makePad("src_0", GST_PAD_SRC).inferTrackTypeFromName() == std::nullopt);
    QVERIFY(makePad("text", GST_PAD_SRC).inferTrackTypeFromName() == std::nullopt);
}

QTEST_GUILESS_MAIN(tst_GStreamer)

#include "moc_tst_gstreamer_backend.cpp"
