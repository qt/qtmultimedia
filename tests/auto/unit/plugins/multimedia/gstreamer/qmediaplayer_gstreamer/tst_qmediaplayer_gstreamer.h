// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_GMEDIAPLAYER_GSTREAMER_H
#define TST_GMEDIAPLAYER_GSTREAMER_H

#include <QtCore/qtemporaryfile.h>
#include <QtCore/qstandardpaths.h>
#include <QtTest/qsignalspy.h>
#include <QtMultimedia/qmediaplayer.h>
#include <QtGstreamerMediaPluginImpl/private/qgstpipeline_p.h>
#include <QtMultimedia/private/qgstreamer_platformspecificinterface_p.h>

#include <memory>
#include <optional>

QT_USE_NAMESPACE

class tst_QMediaPlayerGStreamer : public QObject
{
    Q_OBJECT

public:
    tst_QMediaPlayerGStreamer();

public slots:
    void initTestCase();
    void init();
    void cleanup();

private slots:
    void videoSink_constructor_overridesConversionElement();
    void videoSink_constructor_overridesConversionElement_withMultipleElements();

    void setSource_customGStreamerPipeline_videoTest();
    void setSource_customGStreamerPipeline_uriDecodeBin();

private:
    std::unique_ptr<QMediaPlayer> player;
    std::optional<QSignalSpy> mediaStatusSpy;

    static QGStreamerPlatformSpecificInterface *gstInterface();

    GstPipeline *getGstPipeline();
    QGstPipeline getPipeline();
    void dumpGraph(const char *fileNamePrefix);

    bool mediaSupported;
};

#endif // TST_GMEDIAPLAYER_GSTREAMER_H
