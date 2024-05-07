// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_GSTREAMER_BACKEND_H
#define TST_GSTREAMER_BACKEND_H

#include <QtTest/QtTest>

#include <QtQGstreamerMediaPlugin/private/qgstreamerintegration_p.h>
#include <QtQGstreamerMediaPlugin/private/qgst_handle_types_p.h>

QT_USE_NAMESPACE

class tst_GStreamer : public QObject
{
    Q_OBJECT

    QGstTagListHandle parseTagList(const char *);
    QGstTagListHandle parseTagList(const QByteArray &);

private slots:
    void metadata_taglistToMetaData();
    void metadata_taglistToMetaData_extractsOrientation();
    void metadata_taglistToMetaData_extractsOrientation_data();
    void metadata_taglistToMetaData_extractsDuration();

private:
    QGstreamerIntegration integration;
};

#endif // TST_GSTREAMER_BACKEND_H
