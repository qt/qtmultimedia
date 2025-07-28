// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_GSTREAMER_BACKEND_H
#define TST_GSTREAMER_BACKEND_H

#include <QtTest/qtest.h>

#include <QtGstreamerMediaPluginImpl/private/qgstreamerintegration_p.h>
#include <QtGstreamerMediaPluginImpl/private/qgst_handle_types_p.h>

QT_USE_NAMESPACE

class tst_GStreamer : public QObject
{
    Q_OBJECT

    QGstTagListHandle parseTagList(const char *);
    QGstTagListHandle parseTagList(const QByteArray &);

private slots:
    void QGString_conversions();
    void QGString_transparentCompare();

    void qGstCasts_withElement();
    void qGstCasts_withBin();
    void qGstCasts_withPipeline();

    void metadata_taglistToMetaData();
    void metadata_taglistToMetaData_extractsOrientation();
    void metadata_taglistToMetaData_extractsOrientation_data();
    void metadata_taglistToMetaData_extractsDuration();
    void metadata_taglistToMetaData_extractsLanguage();
    void metadata_taglistToMetaData_extractsLanguage_data();
    void metadata_taglistToMetaData_extractsDate();
    void metadata_taglistToMetaData_extractsDate_data();

    void metadata_capsToMetaData();
    void metadata_capsToMetaData_data();

    void parseRotationTag_returnsCorrectResults();

    void QGstBin_createFromPipelineDescription();
    void QGstElement_createFromPipelineDescription();
    void QGstElement_createFromPipelineDescription_multipleElementsCreatesBin();

    void QGstPad_inferTypeFromName();

    void qDebug_GstPadDirection();
    void qDebug_GstStreamStatusType();

    void QGstStructureView_parseCameraFormat();

    void QGstDiscoverer_discoverMedia();
    void QGstDiscoverer_discoverMedia_data();

    void QGstDiscoverer_discoverMedia_withRotation();
    void QGstDiscoverer_filtersOutVideoStream_whenStreamIdIsNull();

private:
    QGstreamerIntegration integration;
};

#endif // TST_GSTREAMER_BACKEND_H
