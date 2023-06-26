// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <QDebug>
#include <QtMultimedia/qmediametadata.h>
#include <private/qplatformmediarecorder_p.h>
#include "private/qguiapplication_p.h"
#include <qmediarecorder.h>
#include <qaudioformat.h>
#include <qmockintegration.h>
#include <qmediacapturesession.h>
#include <qscreencapture.h>
#include <qwindowcapture.h>

#include "qguiapplication_platform.h"
#include "qmockmediacapturesession.h"
#include "qmockmediaencoder.h"

QT_USE_NAMESPACE

class tst_QMediaRecorder : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void testBasicSession();
    void testNullControls();
    void testDeleteMediaCapture();
    void testError();
    void testSink();
    void testRecord();
    void testEncodingSettings();
    void testAudioSettings();
    void testVideoSettings();
    void testSettingsApplied();

    void metaData();

    void testIsAvailable();
    void testEnum();

    void testVideoSettingsQuality();
    void testVideoSettingsEncodingMode();

    void testApplicationInative();

private:
    QMockIntegrationFactory mockIntegrationFactory;
    QMediaCaptureSession *captureSession;
    QCamera *object = nullptr;
    QMockMediaCaptureSession *service = nullptr;
    QMockMediaEncoder *mock;
    QMediaRecorder *encoder;
};

void tst_QMediaRecorder::initTestCase()
{
    captureSession = new QMediaCaptureSession;
    object = new QCamera;
    encoder = new QMediaRecorder;
    captureSession->setCamera(object);
    captureSession->setRecorder(encoder);
    service = QMockIntegration::instance()->lastCaptureService();
    mock = service->mockControl;
}

void tst_QMediaRecorder::cleanupTestCase()
{
    delete encoder;
    delete object;
    delete captureSession;
}

void tst_QMediaRecorder::testBasicSession()
{
    QMediaCaptureSession session;
    QCamera camera;
    QMediaRecorder recorder;
    session.setCamera(&camera);
    session.setRecorder(&recorder);

    QCOMPARE(recorder.outputLocation(), QUrl());
    QCOMPARE(recorder.recorderState(), QMediaRecorder::StoppedState);
    QCOMPARE(recorder.error(), QMediaRecorder::NoError);
    QCOMPARE(recorder.duration(), qint64(0));
}

void tst_QMediaRecorder::testNullControls()
{
    // With the new changes, hasControls does not make much sense anymore
    // since the session does not own the controls
    // The equivalent of this test would be to not set the control to the session
    // ???
    QMediaCaptureSession session;
    // mockIntegration->lastCaptureService()->hasControls = false;
    QCamera camera;
    QMediaRecorder recorder;
    session.setCamera(&camera);
    // session.setRecorder(&recorder);

    QCOMPARE(recorder.outputLocation(), QUrl());
    QCOMPARE(recorder.recorderState(), QMediaRecorder::StoppedState);
    QCOMPARE(recorder.error(), QMediaRecorder::NoError);
    QCOMPARE(recorder.duration(), qint64(0));

    recorder.setOutputLocation(QUrl("file://test/save/file.mp4"));
    QCOMPARE(recorder.outputLocation(), QUrl("file://test/save/file.mp4"));
    QCOMPARE(recorder.actualLocation(), QUrl());

    QMediaFormat format;
    format.setFileFormat(QMediaFormat::MPEG4);
    format.setAudioCodec(QMediaFormat::AudioCodec::AAC);
    format.setVideoCodec(QMediaFormat::VideoCodec::VP9);
    recorder.setMediaFormat(format);
    recorder.setQuality(QMediaRecorder::LowQuality);
    recorder.setVideoResolution(640, 480);

    QCOMPARE(recorder.mediaFormat().audioCodec(), QMediaFormat::AudioCodec::AAC);
    QCOMPARE(recorder.mediaFormat().videoCodec(), QMediaFormat::VideoCodec::VP9);
    QCOMPARE(recorder.mediaFormat().fileFormat(), QMediaFormat::MPEG4);

    QSignalSpy spy(&recorder, SIGNAL(recorderStateChanged(RecorderState)));

    recorder.record();
    QCOMPARE(recorder.recorderState(), QMediaRecorder::StoppedState);
    QCOMPARE(recorder.error(), QMediaRecorder::NoError);
    QCOMPARE(spy.size(), 0);

    recorder.pause();
    QCOMPARE(recorder.recorderState(), QMediaRecorder::StoppedState);
    QCOMPARE(recorder.error(), QMediaRecorder::NoError);
    QCOMPARE(spy.size(), 0);

    recorder.stop();
    QCOMPARE(recorder.recorderState(), QMediaRecorder::StoppedState);
    QCOMPARE(recorder.error(), QMediaRecorder::NoError);
    QCOMPARE(spy.size(), 0);
}

void tst_QMediaRecorder::testDeleteMediaCapture()
{
    QMediaCaptureSession session;
    QMediaRecorder recorder;

    session.setRecorder(&recorder);

    auto checkSourceDeleting = [&](auto setter, auto getter, auto signal) {
        using Type = std::remove_pointer_t<decltype((session.*getter)())>;

        auto errorPrinter = qScopeGuard(
                []() { qDebug() << QMetaType::fromType<Type>().name() << "deleting failed"; });

        auto capture = std::make_unique<Type>();

        (session.*setter)(capture.get());

        QVERIFY((session.*getter)() == capture.get());

        QSignalSpy spy(&session, signal);
        capture.reset();

        QCOMPARE(spy.size(), 1);
        QCOMPARE((session.*getter)(), nullptr);

        QVERIFY(recorder.isAvailable());

        errorPrinter.dismiss();
    };

    checkSourceDeleting(&QMediaCaptureSession::setImageCapture,
                        &QMediaCaptureSession::imageCapture,
                        &QMediaCaptureSession::imageCaptureChanged);
    checkSourceDeleting(&QMediaCaptureSession::setCamera, &QMediaCaptureSession::camera,
                        &QMediaCaptureSession::cameraChanged);
    checkSourceDeleting(&QMediaCaptureSession::setScreenCapture,
                        &QMediaCaptureSession::screenCapture,
                        &QMediaCaptureSession::screenCaptureChanged);
    checkSourceDeleting(&QMediaCaptureSession::setWindowCapture,
                        &QMediaCaptureSession::windowCapture,
                        &QMediaCaptureSession::windowCaptureChanged);
}

void tst_QMediaRecorder::testError()
{
    const QString errorString(QLatin1String("format error"));

    QSignalSpy spy(encoder, SIGNAL(errorOccurred(Error, const QString&)));

    QCOMPARE(encoder->error(), QMediaRecorder::NoError);
    QCOMPARE(encoder->errorString(), QString());

    mock->error(QMediaRecorder::FormatError, errorString);
    QCOMPARE(encoder->error(), QMediaRecorder::FormatError);
    QCOMPARE(encoder->errorString(), errorString);
    QCOMPARE(spy.size(), 1);

    QCOMPARE(spy.last()[0].value<QMediaRecorder::Error>(), QMediaRecorder::FormatError);
}

void tst_QMediaRecorder::testSink()
{
    encoder->setOutputLocation(QUrl("test.tmp"));
    QUrl s = encoder->outputLocation();
    QCOMPARE(s.toString(), QString("test.tmp"));
    QCOMPARE(encoder->actualLocation(), QUrl());

    //the actual location is available after record
    encoder->record();
    QCOMPARE(encoder->actualLocation().toString(), QString("test.tmp"));
    encoder->stop();
    QCOMPARE(encoder->actualLocation().toString(), QString("test.tmp"));

    //setOutputLocation resets the actual location
    encoder->setOutputLocation(QUrl());
    QCOMPARE(encoder->actualLocation(), QUrl());

    encoder->record();
    QCOMPARE(encoder->actualLocation(), QUrl::fromLocalFile("default_name.mp4"));
    encoder->stop();
    QCOMPARE(encoder->actualLocation(), QUrl::fromLocalFile("default_name.mp4"));

    mock->reset();
}

void tst_QMediaRecorder::testRecord()
{
    QSignalSpy stateSignal(encoder,SIGNAL(recorderStateChanged(RecorderState)));
    QSignalSpy progressSignal(encoder, SIGNAL(durationChanged(qint64)));
    encoder->record();
    QCOMPARE(encoder->recorderState(), QMediaRecorder::RecordingState);
    QCOMPARE(encoder->error(), QMediaRecorder::NoError);
    QCOMPARE(encoder->errorString(), QString());

    QCOMPARE(stateSignal.size(), 1);
    QCOMPARE(stateSignal.last()[0].value<QMediaRecorder::RecorderState>(), QMediaRecorder::RecordingState);

    QTestEventLoop::instance().enterLoop(1);

    QVERIFY(progressSignal.size() > 0);

    encoder->pause();

    QCOMPARE(encoder->recorderState(), QMediaRecorder::PausedState);

    QCOMPARE(stateSignal.size(), 2);

    QTestEventLoop::instance().enterLoop(1);

    encoder->stop();

    QCOMPARE(encoder->recorderState(), QMediaRecorder::StoppedState);
    QCOMPARE(stateSignal.size(), 3);

    QTestEventLoop::instance().enterLoop(1);

    mock->stop();
    QCOMPARE(stateSignal.size(), 3);

    mock->reset();
}

void tst_QMediaRecorder::testEncodingSettings()
{
    QMediaFormat format = encoder->mediaFormat();
    QCOMPARE(format.fileFormat(), QMediaFormat::UnspecifiedFormat);
    QCOMPARE(format.audioCodec(), QMediaFormat::AudioCodec::Unspecified);
    QCOMPARE(format.videoCodec(), QMediaFormat::VideoCodec::Unspecified);
    QCOMPARE(encoder->quality(), QMediaRecorder::NormalQuality);
    QCOMPARE(encoder->encodingMode(), QMediaRecorder::ConstantQualityEncoding);

    format.setAudioCodec(QMediaFormat::AudioCodec::MP3);
    encoder->setAudioSampleRate(44100);
    encoder->setAudioBitRate(256*1024);
    encoder->setQuality(QMediaRecorder::HighQuality);
    encoder->setEncodingMode(QMediaRecorder::AverageBitRateEncoding);

    format.setVideoCodec(QMediaFormat::VideoCodec::H264);
    encoder->setVideoBitRate(800);
    encoder->setVideoFrameRate(24*1024);
    encoder->setVideoResolution(QSize(800,600));
    encoder->setMediaFormat(format);

    QCOMPARE(encoder->mediaFormat().audioCodec(), QMediaFormat::AudioCodec::MP3);
    QCOMPARE(encoder->audioSampleRate(), 44100);
    QCOMPARE(encoder->audioBitRate(), 256*1024);
    QCOMPARE(encoder->quality(), QMediaRecorder::HighQuality);
    QCOMPARE(encoder->encodingMode(), QMediaRecorder::AverageBitRateEncoding);

    QCOMPARE(encoder->mediaFormat().videoCodec(), QMediaFormat::VideoCodec::H264);
    QCOMPARE(encoder->videoBitRate(), 800);
    QCOMPARE(encoder->videoFrameRate(), 24*1024);
    QCOMPARE(encoder->videoResolution(), QSize(800,600));
}

void tst_QMediaRecorder::testAudioSettings()
{
    QMediaRecorder recorder;

    QCOMPARE(recorder.mediaFormat(), QMediaFormat());
    QCOMPARE(recorder.mediaFormat().fileFormat(), QMediaFormat::UnspecifiedFormat);
    QCOMPARE(recorder.audioBitRate(), -1);
    QCOMPARE(recorder.quality(), QMediaRecorder::NormalQuality);
    QCOMPARE(recorder.audioSampleRate(), -1);

    QMediaFormat format;
    format.setFileFormat(QMediaFormat::FLAC);
    recorder.setMediaFormat(format);
    QCOMPARE(recorder.mediaFormat(), format);

    recorder.setAudioBitRate(128000);
    QCOMPARE(recorder.audioBitRate(), 128000);

    recorder.setQuality(QMediaRecorder::HighQuality);
    QCOMPARE(recorder.quality(), QMediaRecorder::HighQuality);

    recorder.setAudioSampleRate(44100);
    QCOMPARE(recorder.audioSampleRate(), 44100);

    QCOMPARE(recorder.audioChannelCount(), -1);
    recorder.setAudioChannelCount(2);
    QCOMPARE(recorder.audioChannelCount(), 2);
}

void tst_QMediaRecorder::testVideoSettings()
{
    QMediaRecorder recorder;

    QCOMPARE(recorder.mediaFormat(), QMediaFormat());
    QCOMPARE(recorder.mediaFormat().videoCodec(), QMediaFormat::VideoCodec::Unspecified);
    QMediaFormat format;
    format.setVideoCodec(QMediaFormat::VideoCodec::H265);
    recorder.setMediaFormat(format);
    QCOMPARE(recorder.mediaFormat(), format);
    QCOMPARE(recorder.mediaFormat().videoCodec(), QMediaFormat::VideoCodec::H265);

    QCOMPARE(recorder.videoBitRate(), -1);
    recorder.setVideoBitRate(128000);
    QCOMPARE(recorder.videoBitRate(), 128000);

    QCOMPARE(recorder.quality(), QMediaRecorder::NormalQuality);
    recorder.setQuality(QMediaRecorder::HighQuality);
    QCOMPARE(recorder.quality(), QMediaRecorder::HighQuality);

    QCOMPARE(recorder.videoFrameRate(), -1);
    recorder.setVideoFrameRate(60);
    QVERIFY(qFuzzyCompare(recorder.videoFrameRate(), qreal(60)));
    recorder.setVideoFrameRate(24.0);
    QVERIFY(qFuzzyCompare(recorder.videoFrameRate(), qreal(24.0)));

    QCOMPARE(recorder.videoResolution(), QSize());
    recorder.setVideoResolution(QSize(320,240));
    QCOMPARE(recorder.videoResolution(), QSize(320,240));
    recorder.setVideoResolution(800,600);
    QCOMPARE(recorder.videoResolution(), QSize(800,600));
}

void tst_QMediaRecorder::testSettingsApplied()
{
    QMediaCaptureSession session;
    QMediaRecorder encoder;
    session.setRecorder(&encoder);

    encoder.setVideoResolution(640,480);

    //encoder settings are applied before recording if changed
    encoder.setQuality(QMediaRecorder::VeryHighQuality);
    encoder.record();

    encoder.stop();
}

void tst_QMediaRecorder::metaData()
{
    QMediaCaptureSession session;
    QCamera camera;
    QMediaRecorder recorder;
    session.setCamera(&camera);
    session.setRecorder(&recorder);

    QVERIFY(recorder.metaData().isEmpty());

    QMediaMetaData data;
    data.insert(QMediaMetaData::Author, QString::fromUtf8("John Doe"));
    recorder.setMetaData(data);

    QCOMPARE(recorder.metaData().value(QMediaMetaData::Author).toString(), QString::fromUtf8("John Doe"));
}

void tst_QMediaRecorder::testIsAvailable()
{
    {
        QMediaCaptureSession session;
        QCamera camera;
        QMediaRecorder recorder;
        session.setCamera(&camera);
        session.setRecorder(&recorder);
        QCOMPARE(recorder.isAvailable(), true);
    }
    {
        QMediaRecorder recorder;
        QCOMPARE(recorder.isAvailable(), false);
    }
}

/* enum QMediaRecorder::ResourceError property test. */
void tst_QMediaRecorder::testEnum()
{
    const QString errorString(QLatin1String("resource error"));

    QSignalSpy spy(encoder, SIGNAL(errorOccurred(Error, const QString&)));

    QCOMPARE(encoder->error(), QMediaRecorder::NoError);
    QCOMPARE(encoder->errorString(), QString());

    emit mock->error(QMediaRecorder::ResourceError, errorString);
    QCOMPARE(encoder->error(), QMediaRecorder::ResourceError);
    QCOMPARE(encoder->errorString(), errorString);
    QCOMPARE(spy.size(), 1);

    QCOMPARE(spy.last()[0].value<QMediaRecorder::Error>(), QMediaRecorder::ResourceError);
}

void tst_QMediaRecorder::testVideoSettingsQuality()
{
    QMediaRecorder recorder;
    /* Verify the default value is intialised correctly*/
    QCOMPARE(recorder.quality(), QMediaRecorder::NormalQuality);

    /* Set all types of Quality parameter and Verify if it is set correctly*/
    recorder.setQuality(QMediaRecorder::HighQuality);
    QCOMPARE(recorder.quality(), QMediaRecorder::HighQuality);

    recorder.setQuality(QMediaRecorder::VeryLowQuality);
    QCOMPARE(recorder.quality(), QMediaRecorder::VeryLowQuality);

    recorder.setQuality(QMediaRecorder::LowQuality);
    QCOMPARE(recorder.quality(), QMediaRecorder::LowQuality);

    recorder.setQuality(QMediaRecorder::VeryHighQuality);
    QCOMPARE(recorder.quality(), QMediaRecorder::VeryHighQuality);
}

void tst_QMediaRecorder::testVideoSettingsEncodingMode()
{
    QMediaRecorder recorder;

    /* Verify the default values are initialised correctly*/
    QCOMPARE(recorder.encodingMode(), QMediaRecorder::ConstantQualityEncoding);

    /* Set each type of encoding mode and Verify if it is set correctly*/
    recorder.setEncodingMode(QMediaRecorder::ConstantBitRateEncoding);
    QCOMPARE(recorder.encodingMode(),QMediaRecorder::ConstantBitRateEncoding);

    recorder.setEncodingMode(QMediaRecorder::AverageBitRateEncoding);
    QCOMPARE(recorder.encodingMode(), QMediaRecorder::AverageBitRateEncoding);
}

void tst_QMediaRecorder::testApplicationInative()
{
    QMediaCaptureSession session;
    QMediaRecorder encoder;
    session.setRecorder(&encoder);

    encoder.setVideoResolution(640, 480);
    encoder.setQuality(QMediaRecorder::VeryHighQuality);

    encoder.setOutputLocation(QUrl("test.tmp"));
    QCOMPARE(encoder.outputLocation().toString(), QString("test.tmp"));
    QCOMPARE(encoder.actualLocation(), QUrl());

    encoder.record();

    QGuiApplicationPrivate::setApplicationState(Qt::ApplicationInactive);
    QCoreApplication::processEvents();

    QGuiApplicationPrivate::setApplicationState(Qt::ApplicationActive);
    QCoreApplication::processEvents();

    encoder.stop();

    // the actual location is available after record
    QCOMPARE(encoder.actualLocation().toString(), QString("test.tmp"));
}

QTEST_GUILESS_MAIN(tst_QMediaRecorder)
#include "tst_qmediarecorder.moc"
