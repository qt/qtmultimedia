// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QAudioDevice>
#include <QtMultimedia/QAudioInput>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/private/qplatformmediacapture_p.h>
#include <QtMultimedia/private/qgstreamer_platformspecificinterface_p.h>
#include <QtQGstreamerMediaPlugin/private/qgstpipeline_p.h>

#include <qscopedenvironmentvariable.h>

#include <memory>

QT_USE_NAMESPACE

using namespace Qt::Literals;

class tst_QMediaCaptureGStreamer : public QObject
{
    Q_OBJECT

public:
    tst_QMediaCaptureGStreamer();

public slots:
    void init();
    void cleanup();

private slots:
    void mediaIntegration_hasPlatformSpecificInterface();
    void constructor_preparesGstPipeline();
    void audioInput_makeCustomGStreamerAudioInput_fromPipelineDescription();
    void audioOutput_makeCustomGStreamerAudioOutput_fromPipelineDescription();

private:
    std::unique_ptr<QMediaCaptureSession> session;

    QGStreamerPlatformSpecificInterface *gstInterface()
    {
        return QGStreamerPlatformSpecificInterface::instance();
    }

    GstPipeline *getGstPipeline()
    {
        auto *iface = QGStreamerPlatformSpecificInterface::instance();
        return iface ? iface->gstPipeline(session.get()) : nullptr;
    }

    QGstPipeline getPipeline()
    {
        return QGstPipeline{
            getGstPipeline(),
            QGstPipeline::NeedsRef,
        };
    }

    void dumpGraph(const char *fileNamePrefix)
    {
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(getGstPipeline()),
                                  GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_VERBOSE),
                                  fileNamePrefix);
    }
};

tst_QMediaCaptureGStreamer::tst_QMediaCaptureGStreamer()
{
    qputenv("QT_MEDIA_BACKEND", "gstreamer");
}

void tst_QMediaCaptureGStreamer::init()
{
    session = std::make_unique<QMediaCaptureSession>();
}

void tst_QMediaCaptureGStreamer::cleanup()
{
    session.reset();
}

void tst_QMediaCaptureGStreamer::mediaIntegration_hasPlatformSpecificInterface()
{
    QVERIFY(QGStreamerPlatformSpecificInterface::instance());
}

void tst_QMediaCaptureGStreamer::constructor_preparesGstPipeline()
{
    auto *rawPipeline = getGstPipeline();
    QVERIFY(rawPipeline);

    QGstPipeline pipeline{
        rawPipeline,
        QGstPipeline::NeedsRef,
    };
    QVERIFY(pipeline);

    dumpGraph("constructor_preparesGstPipeline");
}

void tst_QMediaCaptureGStreamer::audioInput_makeCustomGStreamerAudioInput_fromPipelineDescription()
{
    auto pipelineString =
            "audiotestsrc wave=2 freq=200 name=myOscillator ! identity name=myConverter"_ba;

    QAudioInput input{
        gstInterface()->makeCustomGStreamerAudioInput(pipelineString),
    };

    session->setAudioInput(&input);

    QGstPipeline pipeline = getPipeline();
    QTEST_ASSERT(pipeline);

    pipeline.finishStateChange();

    QVERIFY(pipeline.findByName("myOscillator"));
    QVERIFY(pipeline.findByName("myConverter"));

    dumpGraph("audioInput_customAudioDevice");
}

void tst_QMediaCaptureGStreamer::
        audioOutput_makeCustomGStreamerAudioOutput_fromPipelineDescription()
{
    auto pipelineStringInput =
            "audiotestsrc wave=2 freq=200 name=myOscillator ! identity name=myConverter"_ba;
    QAudioInput input{
        gstInterface()->makeCustomGStreamerAudioInput(pipelineStringInput),
    };
    session->setAudioInput(&input);

    auto pipelineStringOutput = "identity name=myConverter ! fakesink name=mySink"_ba;
    QAudioOutput output{
        gstInterface()->makeCustomGStreamerAudioOutput(pipelineStringOutput),
    };
    session->setAudioOutput(&output);

    QGstPipeline pipeline = getPipeline();
    QTEST_ASSERT(pipeline);

    pipeline.finishStateChange();

    QVERIFY(pipeline.findByName("mySink"));
    QVERIFY(pipeline.findByName("myConverter"));

    dumpGraph("audioOutput_customAudioDevice");
}

QTEST_GUILESS_MAIN(tst_QMediaCaptureGStreamer)

#include "tst_qmediacapture_gstreamer.moc"
