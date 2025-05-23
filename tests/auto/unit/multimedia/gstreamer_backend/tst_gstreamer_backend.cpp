// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tst_gstreamer_backend.h"

#include <QtTest/QtTest>
#include <QtQGstreamerMediaPlugin/private/qgstreamermetadata_p.h>
#include <QtQGstreamerMediaPlugin/private/qgst_handle_types_p.h>

QT_USE_NAMESPACE

using namespace Qt::Literals;

void tst_GStreamer::metadata_fromGstTagList()
{
    QGstTagListHandle tagList{
        gst_tag_list_new_from_string(R"(taglist, title="My Video", comment="yada")"),
        QGstTagListHandle::NeedsRef,
    };

    QGstreamerMetaData parsed = QGstreamerMetaData::fromGstTagList(tagList.get());

    QCOMPARE(parsed.stringValue(QMediaMetaData::Title), u"My Video"_s);
    QCOMPARE(parsed.stringValue(QMediaMetaData::Comment), u"yada"_s);
}

QTEST_GUILESS_MAIN(tst_GStreamer)

#include "moc_tst_gstreamer_backend.cpp"
