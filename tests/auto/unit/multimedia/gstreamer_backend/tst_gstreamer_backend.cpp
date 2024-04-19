// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tst_gstreamer_backend.h"

#include <QtTest/QtTest>
#include <QtQGstreamerMediaPlugin/private/qgstreamermetadata_p.h>
#include <QtQGstreamerMediaPlugin/private/qgst_handle_types_p.h>

QT_USE_NAMESPACE

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

void tst_GStreamer::metadata_fromGstTagList()
{
    QGstTagListHandle tagList = parseTagList(R"(taglist, title="My Video", comment="yada")");

    QGstreamerMetaData parsed = QGstreamerMetaData::fromGstTagList(tagList.get());

    QCOMPARE(parsed.stringValue(QMediaMetaData::Title), u"My Video"_s);
    QCOMPARE(parsed.stringValue(QMediaMetaData::Comment), u"yada"_s);
}

void tst_GStreamer::metadata_fromGstTagList_extractsOrientation()
{
    QFETCH(QByteArray, taglist);
    QFETCH(QtVideo::Rotation, rotation);

    QGstTagListHandle tagList = parseTagList(taglist);
    QGstreamerMetaData parsed = QGstreamerMetaData::fromGstTagList(tagList.get());
    QCOMPARE(parsed[QMediaMetaData::Orientation].value<QtVideo::Rotation>(), rotation);
}

void tst_GStreamer::metadata_fromGstTagList_extractsOrientation_data()
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

void tst_GStreamer::metadata_fromGstTagList_extractsDuration()
{
    QGstTagListHandle tagList = parseTagList(
            R"__(taglist, video-codec=(string)"On2\ VP9",  container-specific-track-id=(string)1, extended-comment=(string){ "ALPHA_MODE\=1", "HANDLER_NAME\=Apple\ Video\ Media\ Handler", "VENDOR_ID\=appl", "TIMECODE\=00:00:00:00", "DURATION\=00:00:00.400000000" }, encoder=(string)"Lavc59.37.100\ libvpx-vp9")__");

    QGstreamerMetaData parsed = QGstreamerMetaData::fromGstTagList(tagList.get());

    QEXPECT_FAIL("", "duration in extended comment", Continue);
    QCOMPARE(parsed[QMediaMetaData::Duration].value<int>(), 400);
}

QTEST_GUILESS_MAIN(tst_GStreamer)

#include "moc_tst_gstreamer_backend.cpp"
