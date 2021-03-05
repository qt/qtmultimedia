/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <QDebug>
#include <QtMultimedia/qmediametadata.h>
#include <private/qplatformmediarecorder_p.h>
#include <qmediaencoder.h>
#include <qaudioformat.h>
#include <qmockintegration_p.h>
#include <qmediacapturesession.h>

#include "mockmediarecorderservice.h"
#include "mockmediarecordercontrol.h"

QT_USE_NAMESPACE

class tst_QMediaEncoder : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void testNullService();
    void testNullControls();
    void testDeleteMediaSource();
    void testError();
    void testSink();
    void testRecord();
    void testEncodingSettings();
    void testAudioSettings();
    void testVideoSettings();
    void testSettingsApplied();

    void metaData();

    void testAudioSettingsCopyConstructor();
    void testAudioSettingsOperatorNotEqual();
    void testAudioSettingsOperatorEqual();
    void testAudioSettingsOperatorAssign();
    void testAudioSettingsDestructor();

    void testIsAvailable();
    void testMediaSource();
    void testEnum();

    void testVideoSettingsQuality();
    void testVideoSettingsEncodingMode();
    void testVideoSettingsCopyConstructor();
    void testVideoSettingsOperatorAssignment();
    void testVideoSettingsOperatorNotEqual();
    void testVideoSettingsOperatorComparison();
    void testVideoSettingsDestructor();

private:
    QMockIntegration *mockIntegration = nullptr;
    QMediaCaptureSession *captureSession;
    QCamera *object = nullptr;
    MockMediaRecorderService *service = nullptr;
    MockMediaRecorderControl *mock;
    QMediaEncoder *capture;
};

void tst_QMediaEncoder::initTestCase()
{
    mockIntegration = new QMockIntegration;
    captureSession = new QMediaCaptureSession;
    object = new QCamera;
    capture = new QMediaEncoder;
    captureSession->setCamera(object);
    captureSession->setEncoder(capture);
    service = mockIntegration->lastCaptureService();
    mock = service->mockControl;
}

void tst_QMediaEncoder::cleanupTestCase()
{
    delete capture;
    delete object;
    delete mock;
    delete mockIntegration;
}

void tst_QMediaEncoder::testNullService()
{
    const QString id(QLatin1String("application/x-format"));

    QMediaCaptureSession session;
    QCamera camera;
    QMediaEncoder recorder;
    session.setCamera(&camera);
    session.setEncoder(&recorder);

    QCOMPARE(recorder.outputLocation(), QUrl());
    QCOMPARE(recorder.state(), QMediaEncoder::StoppedState);
    QCOMPARE(recorder.error(), QMediaEncoder::NoError);
    QCOMPARE(recorder.duration(), qint64(0));
}

void tst_QMediaEncoder::testNullControls()
{
    service->hasControls = false;
    QMediaCaptureSession session;
    QCamera camera;
    QMediaEncoder recorder;
    session.setCamera(&camera);
    session.setEncoder(&recorder);

    QCOMPARE(recorder.outputLocation(), QUrl());
    QCOMPARE(recorder.state(), QMediaEncoder::StoppedState);
    QCOMPARE(recorder.error(), QMediaEncoder::NoError);
    QCOMPARE(recorder.duration(), qint64(0));

    recorder.setOutputLocation(QUrl("file://test/save/file.mp4"));
    QCOMPARE(recorder.outputLocation(), QUrl());
    QCOMPARE(recorder.actualLocation(), QUrl());

    QMediaEncoderSettings settings;
    settings.setFormat(QMediaFormat::MPEG4);
    settings.setAudioCodec(QMediaFormat::AudioCodec::AAC);
    settings.setQuality(QMediaEncoderSettings::LowQuality);

    settings.setVideoCodec(QMediaFormat::VideoCodec::VP9);
    settings.setVideoResolution(640, 480);

    recorder.setEncoderSettings(settings);

    QCOMPARE(recorder.encoderSettings().audioCodec(), QMediaFormat::AudioCodec::AAC);
    QCOMPARE(recorder.encoderSettings().videoCodec(), QMediaFormat::VideoCodec::VP9);
    QCOMPARE(recorder.encoderSettings().format(), QMediaFormat::MPEG4);

    QSignalSpy spy(&recorder, SIGNAL(stateChanged(QMediaEncoder::State)));

    recorder.record();
    QCOMPARE(recorder.state(), QMediaEncoder::StoppedState);
    QCOMPARE(recorder.error(), QMediaEncoder::NoError);
    QCOMPARE(spy.count(), 0);

    recorder.pause();
    QCOMPARE(recorder.state(), QMediaEncoder::StoppedState);
    QCOMPARE(recorder.error(), QMediaEncoder::NoError);
    QCOMPARE(spy.count(), 0);

    recorder.stop();
    QCOMPARE(recorder.state(), QMediaEncoder::StoppedState);
    QCOMPARE(recorder.error(), QMediaEncoder::NoError);
    QCOMPARE(spy.count(), 0);
}

void tst_QMediaEncoder::testDeleteMediaSource()
{
    QMediaCaptureSession session;
    QCamera *camera = new QCamera;
    QMediaEncoder *recorder = new QMediaEncoder;
    session.setCamera(camera);
    session.setEncoder(recorder);

    QVERIFY(session.camera() == camera);
    QVERIFY(recorder->isAvailable());

    delete camera;

    QVERIFY(session.camera() == nullptr);
    QVERIFY(recorder->isAvailable());

    delete recorder;
}

void tst_QMediaEncoder::testError()
{
    const QString errorString(QLatin1String("format error"));

    QSignalSpy spy(capture, SIGNAL(error(QMediaEncoder::Error)));

    QCOMPARE(capture->error(), QMediaEncoder::NoError);
    QCOMPARE(capture->errorString(), QString());

    mock->error(QMediaEncoder::FormatError, errorString);
    QCOMPARE(capture->error(), QMediaEncoder::FormatError);
    QCOMPARE(capture->errorString(), errorString);
    QCOMPARE(spy.count(), 1);

    QCOMPARE(spy.last()[0].value<QMediaEncoder::Error>(), QMediaEncoder::FormatError);
}

void tst_QMediaEncoder::testSink()
{
    capture->setOutputLocation(QUrl("test.tmp"));
    QUrl s = capture->outputLocation();
    QCOMPARE(s.toString(), QString("test.tmp"));
    QCOMPARE(capture->actualLocation(), QUrl());

    //the actual location is available after record
    capture->record();
    QCOMPARE(capture->actualLocation().toString(), QString("test.tmp"));
    capture->stop();
    QCOMPARE(capture->actualLocation().toString(), QString("test.tmp"));

    //setOutputLocation resets the actual location
    capture->setOutputLocation(QUrl());
    QCOMPARE(capture->actualLocation(), QUrl());

    capture->record();
    QCOMPARE(capture->actualLocation(), QUrl::fromLocalFile("default_name.mp4"));
    capture->stop();
    QCOMPARE(capture->actualLocation(), QUrl::fromLocalFile("default_name.mp4"));
}

void tst_QMediaEncoder::testRecord()
{
    QSignalSpy stateSignal(capture,SIGNAL(stateChanged(QMediaEncoder::State)));
    QSignalSpy statusSignal(capture,SIGNAL(statusChanged(QMediaEncoder::Status)));
    QSignalSpy progressSignal(capture, SIGNAL(durationChanged(qint64)));
    capture->record();
    QCOMPARE(capture->state(), QMediaEncoder::RecordingState);
    QCOMPARE(capture->error(), QMediaEncoder::NoError);
    QCOMPARE(capture->errorString(), QString());

    QCOMPARE(stateSignal.count(), 1);
    QCOMPARE(stateSignal.last()[0].value<QMediaEncoder::State>(), QMediaEncoder::RecordingState);

    QTestEventLoop::instance().enterLoop(1);

    QCOMPARE(capture->status(), QMediaEncoder::RecordingStatus);
    QVERIFY(!statusSignal.isEmpty());
    QCOMPARE(statusSignal.last()[0].value<QMediaEncoder::Status>(), QMediaEncoder::RecordingStatus);
    statusSignal.clear();

    QVERIFY(progressSignal.count() > 0);

    capture->pause();

    QCOMPARE(capture->state(), QMediaEncoder::PausedState);

    QCOMPARE(stateSignal.count(), 2);

    QTestEventLoop::instance().enterLoop(1);
    QCOMPARE(capture->status(), QMediaEncoder::PausedStatus);
    QVERIFY(!statusSignal.isEmpty());
    QCOMPARE(statusSignal.last()[0].value<QMediaEncoder::Status>(), QMediaEncoder::PausedStatus);
    statusSignal.clear();

    capture->stop();

    QCOMPARE(capture->state(), QMediaEncoder::StoppedState);
    QCOMPARE(stateSignal.count(), 3);

    QTestEventLoop::instance().enterLoop(1);
    QCOMPARE(capture->status(), QMediaEncoder::StoppedStatus);
    QVERIFY(!statusSignal.isEmpty());
    QCOMPARE(statusSignal.last()[0].value<QMediaEncoder::Status>(), QMediaEncoder::StoppedStatus);
    statusSignal.clear();

    mock->stop();
    QCOMPARE(stateSignal.count(), 3);
}

void tst_QMediaEncoder::testEncodingSettings()
{
//    QAudioEncoderSettings audioSettings = capture->audioSettings();
//    QCOMPARE(audioSettings.codec(), QString("audio/x-raw"));
//    QCOMPARE(audioSettings.bitRate(), 128*1024);
//    QCOMPARE(audioSettings.sampleRate(), 8000);
//    QCOMPARE(audioSettings.quality(), QMediaEncoderSettings:NormalQuality);
//    QCOMPARE(audioSettings.channelCount(), -1);

//    QCOMPARE(audioSettings.encodingMode(), QMediaEncoderSettings:ConstantQualityEncoding);

//    QVideoEncoderSettings videoSettings = capture->videoSettings();
//    QCOMPARE(videoSettings.codec(), QString());
//    QCOMPARE(videoSettings.bitRate(), -1);
//    QCOMPARE(videoSettings.resolution(), QSize());
//    QCOMPARE(videoSettings.frameRate(), 0.0);
//    QCOMPARE(videoSettings.quality(), QMediaEncoderSettings:NormalQuality);
//    QCOMPARE(videoSettings.encodingMode(), QMediaEncoderSettings:ConstantQualityEncoding);

//    QString format = capture->containerFormat();
//    QCOMPARE(format, QString());

//    audioSettings.setCodec("audio/mpeg");
//    audioSettings.setSampleRate(44100);
//    audioSettings.setBitRate(256*1024);
//    audioSettings.setQuality(QMediaEncoderSettings:HighQuality);
//    audioSettings.setEncodingMode(QMediaEncoderSettings:AverageBitRateEncoding);

//    videoSettings.setCodec("video/3gpp");
//    videoSettings.setBitRate(800);
//    videoSettings.setFrameRate(24*1024);
//    videoSettings.setResolution(QSize(800,600));
//    videoSettings.setQuality(QMediaEncoderSettings:HighQuality);
//    audioSettings.setEncodingMode(QMediaEncoderSettings:TwoPassEncoding);

//    format = QString("mov");

//    capture->setAudioSettings(audioSettings);
//    capture->setVideoSettings(videoSettings);
//    capture->setContainerFormat(format);

//    QCOMPARE(capture->audioSettings(), audioSettings);
//    QCOMPARE(capture->videoSettings(), videoSettings);
//    QCOMPARE(capture->containerFormat(), format);
}

void tst_QMediaEncoder::testAudioSettings()
{
//    QAudioEncoderSettings settings;
//    QVERIFY(settings.isNull());
//    QVERIFY(settings == QAudioEncoderSettings());

//    QCOMPARE(settings.codec(), QString());
//    settings.setCodec(QLatin1String("codecName"));
//    QCOMPARE(settings.codec(), QLatin1String("codecName"));
//    QVERIFY(!settings.isNull());
//    QVERIFY(settings != QAudioEncoderSettings());

//    settings = QAudioEncoderSettings();
//    QCOMPARE(settings.bitRate(), -1);
//    settings.setBitRate(128000);
//    QCOMPARE(settings.bitRate(), 128000);
//    QVERIFY(!settings.isNull());

//    settings = QAudioEncoderSettings();
//    QCOMPARE(settings.quality(), QMediaEncoderSettings:NormalQuality);
//    settings.setQuality(QMediaEncoderSettings:HighQuality);
//    QCOMPARE(settings.quality(), QMediaEncoderSettings:HighQuality);
//    QVERIFY(!settings.isNull());

//    settings = QAudioEncoderSettings();
//    QCOMPARE(settings.sampleRate(), -1);
//    settings.setSampleRate(44100);
//    QCOMPARE(settings.sampleRate(), 44100);
//    QVERIFY(!settings.isNull());

//    settings = QAudioEncoderSettings();
//    QCOMPARE(settings.channelCount(), -1);
//    settings.setChannelCount(2);
//    QCOMPARE(settings.channelCount(), 2);
//    QVERIFY(!settings.isNull());

//    settings = QAudioEncoderSettings();
//    settings.setEncodingOption(QLatin1String("encoderOption"), QVariant(1));
//    QCOMPARE(settings.encodingOption(QLatin1String("encoderOption")), QVariant(1));
//    QVariantMap options;
//    options.insert(QLatin1String("encoderOption"), QVariant(1));
//    QCOMPARE(settings.encodingOptions(), options);
//    options.insert(QLatin1String("encoderOption2"), QVariant(2));
//    options.remove(QLatin1String("encoderOption"));
//    settings.setEncodingOptions(options);
//    QCOMPARE(settings.encodingOption(QLatin1String("encoderOption")), QVariant());
//    QCOMPARE(settings.encodingOption(QLatin1String("encoderOption2")), QVariant(2));
//    QVERIFY(!settings.isNull());
//    QVERIFY(settings != QAudioEncoderSettings());

//    settings = QAudioEncoderSettings();
//    QVERIFY(settings.isNull());
//    QCOMPARE(settings.codec(), QString());
//    QCOMPARE(settings.bitRate(), -1);
//    QCOMPARE(settings.quality(), QMediaEncoderSettings:NormalQuality);
//    QCOMPARE(settings.sampleRate(), -1);
//    QVERIFY(settings.encodingOptions().isEmpty());

//    {
//        QAudioEncoderSettings settings1;
//        QAudioEncoderSettings settings2;
//        QCOMPARE(settings2, settings1);

//        settings2 = settings1;
//        QCOMPARE(settings2, settings1);
//        QVERIFY(settings2.isNull());

//        settings1.setQuality(QMediaEncoderSettings:HighQuality);

//        QVERIFY(settings2.isNull());
//        QVERIFY(!settings1.isNull());
//        QVERIFY(settings1 != settings2);
//    }

//    {
//        QAudioEncoderSettings settings1;
//        QAudioEncoderSettings settings2(settings1);
//        QCOMPARE(settings2, settings1);

//        settings2 = settings1;
//        QCOMPARE(settings2, settings1);
//        QVERIFY(settings2.isNull());

//        settings1.setQuality(QMediaEncoderSettings:HighQuality);

//        QVERIFY(settings2.isNull());
//        QVERIFY(!settings1.isNull());
//        QVERIFY(settings1 != settings2);
//    }

//    QAudioEncoderSettings settings1;
//    settings1.setBitRate(1);
//    QAudioEncoderSettings settings2;
//    settings2.setBitRate(1);
//    QVERIFY(settings1 == settings2);
//    settings2.setBitRate(2);
//    QVERIFY(settings1 != settings2);

//    settings1 = QAudioEncoderSettings();
//    settings1.setChannelCount(1);
//    settings2 = QAudioEncoderSettings();
//    settings2.setChannelCount(1);
//    QVERIFY(settings1 == settings2);
//    settings2.setChannelCount(2);
//    QVERIFY(settings1 != settings2);

//    settings1 = QAudioEncoderSettings();
//    settings1.setCodec("codec1");
//    settings2 = QAudioEncoderSettings();
//    settings2.setCodec("codec1");
//    QVERIFY(settings1 == settings2);
//    settings2.setCodec("codec2");
//    QVERIFY(settings1 != settings2);

//    settings1 = QAudioEncoderSettings();
//    settings1.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    settings2 = QAudioEncoderSettings();
//    settings2.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    QVERIFY(settings1 == settings2);
//    settings2.setEncodingMode(QMediaEncoderSettings:TwoPassEncoding);
//    QVERIFY(settings1 != settings2);

//    settings1 = QAudioEncoderSettings();
//    settings1.setQuality(QMediaEncoderSettings:NormalQuality);
//    settings2 = QAudioEncoderSettings();
//    settings2.setQuality(QMediaEncoderSettings:NormalQuality);
//    QVERIFY(settings1 == settings2);
//    settings2.setQuality(QMediaEncoderSettings:LowQuality);
//    QVERIFY(settings1 != settings2);

//    settings1 = QAudioEncoderSettings();
//    settings1.setSampleRate(1);
//    settings2 = QAudioEncoderSettings();
//    settings2.setSampleRate(1);
//    QVERIFY(settings1 == settings2);
//    settings2.setSampleRate(2);
//    QVERIFY(settings1 != settings2);

//    settings1 = QAudioEncoderSettings();
//    settings1.setEncodingOption(QLatin1String("encoderOption"), QVariant(1));
//    settings2 = QAudioEncoderSettings();
//    settings2.setEncodingOption(QLatin1String("encoderOption"), QVariant(1));
//    QVERIFY(settings1 == settings2);
//    settings2.setEncodingOption(QLatin1String("encoderOption"), QVariant(2));
//    QVERIFY(settings1 != settings2);
}

void tst_QMediaEncoder::testVideoSettings()
{
//    QVideoEncoderSettings settings;
//    QVERIFY(settings.isNull());
//    QVERIFY(settings == QVideoEncoderSettings());

//    QCOMPARE(settings.codec(), QString());
//    settings.setCodec(QLatin1String("codecName"));
//    QCOMPARE(settings.codec(), QLatin1String("codecName"));
//    QVERIFY(!settings.isNull());
//    QVERIFY(settings != QVideoEncoderSettings());

//    settings = QVideoEncoderSettings();
//    QCOMPARE(settings.bitRate(), -1);
//    settings.setBitRate(128000);
//    QCOMPARE(settings.bitRate(), 128000);
//    QVERIFY(!settings.isNull());

//    settings = QVideoEncoderSettings();
//    QCOMPARE(settings.quality(), QMediaEncoderSettings:NormalQuality);
//    settings.setQuality(QMediaEncoderSettings:HighQuality);
//    QCOMPARE(settings.quality(), QMediaEncoderSettings:HighQuality);
//    QVERIFY(!settings.isNull());

//    settings = QVideoEncoderSettings();
//    QCOMPARE(settings.frameRate(), qreal());
//    settings.setFrameRate(30000.0/10001);
//    QVERIFY(qFuzzyCompare(settings.frameRate(), qreal(30000.0/10001)));
//    settings.setFrameRate(24.0);
//    QVERIFY(qFuzzyCompare(settings.frameRate(), qreal(24.0)));
//    QVERIFY(!settings.isNull());

//    settings = QVideoEncoderSettings();
//    QCOMPARE(settings.resolution(), QSize());
//    settings.setResolution(QSize(320,240));
//    QCOMPARE(settings.resolution(), QSize(320,240));
//    settings.setResolution(800,600);
//    QCOMPARE(settings.resolution(), QSize(800,600));
//    QVERIFY(!settings.isNull());

//    settings = QVideoEncoderSettings();
//    settings.setEncodingOption(QLatin1String("encoderOption"), QVariant(1));
//    QCOMPARE(settings.encodingOption(QLatin1String("encoderOption")), QVariant(1));
//    QVariantMap options;
//    options.insert(QLatin1String("encoderOption"), QVariant(1));
//    QCOMPARE(settings.encodingOptions(), options);
//    options.insert(QLatin1String("encoderOption2"), QVariant(2));
//    options.remove(QLatin1String("encoderOption"));
//    settings.setEncodingOptions(options);
//    QCOMPARE(settings.encodingOption(QLatin1String("encoderOption")), QVariant());
//    QCOMPARE(settings.encodingOption(QLatin1String("encoderOption2")), QVariant(2));
//    QVERIFY(!settings.isNull());
//    QVERIFY(settings != QVideoEncoderSettings());

//    settings = QVideoEncoderSettings();
//    QVERIFY(settings.isNull());
//    QCOMPARE(settings.codec(), QString());
//    QCOMPARE(settings.bitRate(), -1);
//    QCOMPARE(settings.quality(), QMediaEncoderSettings:NormalQuality);
//    QCOMPARE(settings.frameRate(), qreal());
//    QCOMPARE(settings.resolution(), QSize());
//    QVERIFY(settings.encodingOptions().isEmpty());

//    {
//        QVideoEncoderSettings settings1;
//        QVideoEncoderSettings settings2;
//        QCOMPARE(settings2, settings1);

//        settings2 = settings1;
//        QCOMPARE(settings2, settings1);
//        QVERIFY(settings2.isNull());

//        settings1.setQuality(QMediaEncoderSettings:HighQuality);

//        QVERIFY(settings2.isNull());
//        QVERIFY(!settings1.isNull());
//        QVERIFY(settings1 != settings2);
//    }

//    {
//        QVideoEncoderSettings settings1;
//        QVideoEncoderSettings settings2(settings1);
//        QCOMPARE(settings2, settings1);

//        settings2 = settings1;
//        QCOMPARE(settings2, settings1);
//        QVERIFY(settings2.isNull());

//        settings1.setQuality(QMediaEncoderSettings:HighQuality);

//        QVERIFY(settings2.isNull());
//        QVERIFY(!settings1.isNull());
//        QVERIFY(settings1 != settings2);
//    }

//    QVideoEncoderSettings settings1;
//    settings1.setBitRate(1);
//    QVideoEncoderSettings settings2;
//    settings2.setBitRate(1);
//    QVERIFY(settings1 == settings2);
//    settings2.setBitRate(2);
//    QVERIFY(settings1 != settings2);

//    settings1 = QVideoEncoderSettings();
//    settings1.setResolution(800,600);
//    settings2 = QVideoEncoderSettings();
//    settings2.setResolution(QSize(800,600));
//    QVERIFY(settings1 == settings2);
//    settings2.setResolution(QSize(400,300));
//    QVERIFY(settings1 != settings2);

//    settings1 = QVideoEncoderSettings();
//    settings1.setCodec("codec1");
//    settings2 = QVideoEncoderSettings();
//    settings2.setCodec("codec1");
//    QVERIFY(settings1 == settings2);
//    settings2.setCodec("codec2");
//    QVERIFY(settings1 != settings2);

//    settings1 = QVideoEncoderSettings();
//    settings1.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    settings2 = QVideoEncoderSettings();
//    settings2.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    QVERIFY(settings1 == settings2);
//    settings2.setEncodingMode(QMediaEncoderSettings:TwoPassEncoding);
//    QVERIFY(settings1 != settings2);

//    settings1 = QVideoEncoderSettings();
//    settings1.setQuality(QMediaEncoderSettings:NormalQuality);
//    settings2 = QVideoEncoderSettings();
//    settings2.setQuality(QMediaEncoderSettings:NormalQuality);
//    QVERIFY(settings1 == settings2);
//    settings2.setQuality(QMediaEncoderSettings:LowQuality);
//    QVERIFY(settings1 != settings2);

//    settings1 = QVideoEncoderSettings();
//    settings1.setFrameRate(1);
//    settings2 = QVideoEncoderSettings();
//    settings2.setFrameRate(1);
//    QVERIFY(settings1 == settings2);
//    settings2.setFrameRate(2);
//    QVERIFY(settings1 != settings2);

//    settings1 = QVideoEncoderSettings();
//    settings1.setEncodingOption(QLatin1String("encoderOption"), QVariant(1));
//    settings2 = QVideoEncoderSettings();
//    settings2.setEncodingOption(QLatin1String("encoderOption"), QVariant(1));
//    QVERIFY(settings1 == settings2);
//    settings2.setEncodingOption(QLatin1String("encoderOption"), QVariant(2));
//    QVERIFY(settings1 != settings2);
}

void tst_QMediaEncoder::testSettingsApplied()
{
//    MockMediaSource object(nullptr, service);

//    //if the media recorder is not configured after construction
//    //the settings are applied in the next event loop
//    QMediaEncoder recorder(&object);
//    QCOMPARE(mock->m_settingAppliedCount, 0);
//    QTRY_COMPARE(mock->m_settingAppliedCount, 1);

//    QVideoEncoderSettings videoSettings;
//    videoSettings.setResolution(640,480);
//    recorder.setVideoSettings(videoSettings);

//    QAudioEncoderSettings audioSettings;
//    audioSettings.setQuality(QMediaEncoderSettings:HighQuality);
//    recorder.setAudioSettings(audioSettings);

//    recorder.setContainerFormat("mkv");

//    QCOMPARE(mock->m_settingAppliedCount, 1);
//    QTRY_COMPARE(mock->m_settingAppliedCount, 2);

//    //encoder settings are applied before recording if changed
//    audioSettings.setQuality(QMediaEncoderSettings:VeryHighQuality);
//    recorder.setAudioSettings(audioSettings);

//    QCOMPARE(mock->m_settingAppliedCount, 2);
//    recorder.record();
//    QCOMPARE(mock->m_settingAppliedCount, 3);

//    recorder.stop();

//    //applySettings is not called if setting has not changes
//    recorder.record();
//    QCOMPARE(mock->m_settingAppliedCount, 3);
}

void tst_QMediaEncoder::metaData()
{
    QFETCH(QString, artist);
    QFETCH(QString, title);
    QFETCH(QString, genre);
    QFETCH(QString, custom);

    QMediaCaptureSession session;
    QCamera camera;
    QMediaEncoder recorder;
    session.setCamera(&camera);
    session.setEncoder(&recorder);

    QVERIFY(recorder.metaData().isEmpty());

    QMediaMetaData data;
    data.insert(QMediaMetaData::Author, QString::fromUtf8("John Doe"));
    recorder.setMetaData(data);

    QCOMPARE(recorder.metaData().value(QMediaMetaData::Author).toString(), QString::fromUtf8("John Doe"));
}

void tst_QMediaEncoder::testAudioSettingsCopyConstructor()
{
    /* create an object for AudioEncodersettings */
//    QAudioEncoderSettings audiosettings;
//    QVERIFY(audiosettings.isNull());

//    /* setting the desired properties for the AudioEncoder */
//    audiosettings.setBitRate(128*1000);
//    audiosettings.setChannelCount(4);
//    audiosettings.setCodec("audio/x-raw");
//    audiosettings.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    audiosettings.setQuality(QMediaEncoderSettings:LowQuality);
//    audiosettings.setSampleRate(44100);

//    /* Copy constructor */
//    QAudioEncoderSettings other(audiosettings);
//    QVERIFY(!(other.isNull()));

//    /* Verifying whether data is copied properly or not */
//    QVERIFY(other.bitRate() == audiosettings.bitRate());
//    QVERIFY(other.sampleRate() == audiosettings.sampleRate());
//    QVERIFY(other.channelCount() == audiosettings.channelCount());
//    QCOMPARE(other.codec(), audiosettings.codec());
//    QVERIFY(other.encodingMode() == audiosettings.encodingMode());
//    QVERIFY(other.quality() == audiosettings.quality());
}

void tst_QMediaEncoder::testAudioSettingsOperatorNotEqual()
{
    /* create an object for AudioEncodersettings */
//    QAudioEncoderSettings audiosettings1;
//    QVERIFY(audiosettings1.isNull());

//    QAudioEncoderSettings audiosettings2;
//    QVERIFY(audiosettings2.isNull());

//    /* setting the desired properties to for the AudioEncoder */
//    audiosettings1.setBitRate(128*1000);
//    audiosettings1.setChannelCount(4);
//    audiosettings1.setCodec("audio/x-raw");
//    audiosettings1.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    audiosettings1.setQuality(QMediaEncoderSettings:LowQuality);
//    audiosettings1.setSampleRate(44100);

//    /* setting the desired properties for the AudioEncoder */
//    audiosettings2.setBitRate(128*1000);
//    audiosettings2.setChannelCount(4);
//    audiosettings2.setCodec("audio/x-raw");
//    audiosettings2.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    audiosettings2.setQuality(QMediaEncoderSettings:LowQuality);
//    audiosettings2.setSampleRate(44100);

//    /* verify the both are equal or not */
//    QVERIFY(!(audiosettings1 != audiosettings2));

//    /* Modify the settings value for one object */
//    audiosettings2.setBitRate(64*1000);
//    audiosettings2.setEncodingMode(QMediaEncoderSettings:ConstantQualityEncoding);

//    /* verify the not equal opertor */
//    QVERIFY(audiosettings1 != audiosettings2);

//    QVERIFY(audiosettings2.bitRate() != audiosettings1.bitRate());
//    QVERIFY(audiosettings2.encodingMode() != audiosettings1.encodingMode());
}

void tst_QMediaEncoder::testAudioSettingsOperatorEqual()
{
    /* create an object for AudioEncodersettings */
//    QAudioEncoderSettings audiosettings1;
//    QVERIFY(audiosettings1.isNull());

//    /* setting the desired properties to for the AudioEncoder */
//    audiosettings1.setBitRate(128*1000);
//    audiosettings1.setChannelCount(4);
//    audiosettings1.setCodec("audio/x-raw");
//    audiosettings1.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    audiosettings1.setQuality(QMediaEncoderSettings:LowQuality);
//    audiosettings1.setSampleRate(44100);

//    QAudioEncoderSettings audiosettings2;
//    QVERIFY(audiosettings2.isNull());

//    /* setting the desired properties for the AudioEncoder */
//    audiosettings2.setBitRate(128*1000);
//    audiosettings2.setChannelCount(4);
//    audiosettings2.setCodec("audio/x-raw");
//    audiosettings2.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    audiosettings2.setQuality(QMediaEncoderSettings:LowQuality);
//    audiosettings2.setSampleRate(44100);

//    /* verify both the values are same or not */
//    QVERIFY(audiosettings1 == audiosettings2);
//    audiosettings2.setChannelCount(2);
//    QVERIFY(audiosettings1 != audiosettings2);
}

void tst_QMediaEncoder::testAudioSettingsOperatorAssign()
{

    /* create an object for AudioEncodersettings */
//    QAudioEncoderSettings audiosettings1;
//    QVERIFY(audiosettings1.isNull());

//    /* setting the desired properties for the AudioEncoder */
//    audiosettings1.setBitRate(128*1000);
//    audiosettings1.setChannelCount(4);
//    audiosettings1.setCodec("audio/x-raw");
//    audiosettings1.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    audiosettings1.setQuality(QMediaEncoderSettings:LowQuality);
//    audiosettings1.setSampleRate(44100);

//    QAudioEncoderSettings audiosettings2;
//    audiosettings2 = audiosettings1;
//    /* Verifying whether data is copied properly or not */
//    QVERIFY(audiosettings2.bitRate() == audiosettings1.bitRate());
//    QVERIFY(audiosettings2.sampleRate() == audiosettings1.sampleRate());
//    QVERIFY(audiosettings2.channelCount() == audiosettings1.channelCount());
//    QCOMPARE(audiosettings2.codec(), audiosettings1.codec());
//    QVERIFY(audiosettings2.encodingMode() == audiosettings1.encodingMode());
//    QVERIFY(audiosettings2.quality() == audiosettings1.quality());
}

void tst_QMediaEncoder::testAudioSettingsDestructor()
{
    /* Creating null object for the audioencodersettings */
//    QAudioEncoderSettings * audiosettings = new QAudioEncoderSettings;

//   /* Verifying the object is null or not */
//    QVERIFY(audiosettings->isNull());
//    /* delete the allocated memory */
//    delete audiosettings;
}

void tst_QMediaEncoder::testIsAvailable()
{
    {
        QMediaCaptureSession session;
        QCamera camera;
        QMediaEncoder recorder;
        session.setCamera(&camera);
        session.setEncoder(&recorder);
        QCOMPARE(recorder.isAvailable(), true);
    }
    {
        QMediaEncoder recorder;
        QCOMPARE(recorder.isAvailable(), false);
    }
}

/* mediaSource() API test. */
void tst_QMediaEncoder::testMediaSource()
{
    service->hasControls = false;
    QMediaCaptureSession session;
    QCamera camera;
    QMediaEncoder recorder;
    session.setCamera(&camera);
    session.setEncoder(&recorder);

    QCamera *medobj = session.camera();
    QVERIFY(medobj != nullptr);
    QVERIFY(!camera.isAvailable());
}

/* enum QMediaEncoder::ResourceError property test. */
void tst_QMediaEncoder::testEnum()
{
    const QString errorString(QLatin1String("resource error"));

    QSignalSpy spy(capture, SIGNAL(error(QMediaEncoder::Error)));

    QCOMPARE(capture->error(), QMediaEncoder::NoError);
    QCOMPARE(capture->errorString(), QString());

    mock->error(QMediaEncoder::ResourceError, errorString);
    QCOMPARE(capture->error(), QMediaEncoder::ResourceError);
    QCOMPARE(capture->errorString(), errorString);
    QCOMPARE(spy.count(), 1);

    QCOMPARE(spy.last()[0].value<QMediaEncoder::Error>(), QMediaEncoder::ResourceError);
}

/* Test the QVideoEncoderSettings quality API*/
void tst_QMediaEncoder::testVideoSettingsQuality()
{
//    /* Create the instance*/
//    QVideoEncoderSettings settings;
//    QVERIFY(settings.isNull());
//    QVERIFY(settings == QVideoEncoderSettings());

//    /* Verify the default value is intialised correctly*/
//    QCOMPARE(settings.quality(), QMediaEncoderSettings:NormalQuality);

//    /* Set all types of Quality parameter and Verify if it is set correctly*/
//    settings.setQuality(QMediaEncoderSettings:HighQuality);
//    QCOMPARE(settings.quality(), QMediaEncoderSettings:HighQuality);
//    QVERIFY(!settings.isNull());

//    settings.setQuality(QMediaEncoderSettings:VeryLowQuality);
//    QCOMPARE(settings.quality(), QMediaEncoderSettings:VeryLowQuality);

//    settings.setQuality(QMediaEncoderSettings:LowQuality);
//    QCOMPARE(settings.quality(), QMediaEncoderSettings:LowQuality);

//    settings.setQuality(QMediaEncoderSettings:VeryHighQuality);
//    QCOMPARE(settings.quality(), QMediaEncoderSettings:VeryHighQuality);
}

/* Test  QVideoEncoderSettings encodingMode */
void tst_QMediaEncoder::testVideoSettingsEncodingMode()
{
//    /* Create the instance*/
//    QVideoEncoderSettings settings;
//    QVERIFY(settings.isNull());
//    QVERIFY(settings == QVideoEncoderSettings());

//    /* Verify the default values are initialised correctly*/
//    QCOMPARE(settings.encodingMode(), QMediaEncoderSettings:ConstantQualityEncoding);
//    QVERIFY(settings.isNull());

//    /* Set each type of encoding mode and Verify if it is set correctly*/
//    settings.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    QCOMPARE(settings.encodingMode(),QMediaEncoderSettings:ConstantBitRateEncoding);
//    QVERIFY(!settings.isNull());

//    settings.setEncodingMode(QMediaEncoderSettings:AverageBitRateEncoding);
//    QCOMPARE(settings.encodingMode(), QMediaEncoderSettings:AverageBitRateEncoding);

//    settings.setEncodingMode(QMediaEncoderSettings:TwoPassEncoding);
//    QCOMPARE(settings.encodingMode(), QMediaEncoderSettings:TwoPassEncoding);
}

/* Test QVideoEncoderSettings copy constructor */
void tst_QMediaEncoder::testVideoSettingsCopyConstructor()
{
//    /* Create the instance and initialise it*/
//    QVideoEncoderSettings settings1;
//    settings1.setCodec(QLatin1String("codecName"));
//    settings1.setBitRate(128000);
//    settings1.setQuality(QMediaEncoderSettings:HighQuality);
//    settings1.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    settings1.setFrameRate(30000.0/10001);
//    settings1.setResolution(QSize(320,240));

//    /* Create another instance with instance1 as argument*/
//    QVideoEncoderSettings settings2(settings1);

//    /* Verify if all the parameters are copied correctly*/
//    QCOMPARE(settings2 != settings1, false);
//    QCOMPARE(settings2.codec(), QLatin1String("codecName"));
//    QCOMPARE(settings2.bitRate(), 128000);
//    QCOMPARE(settings2.encodingMode(), QMediaEncoderSettings:ConstantBitRateEncoding);
//    QVERIFY(qFuzzyCompare(settings2.frameRate(), qreal(30000.0/10001)));
//    QCOMPARE(settings2.resolution(), QSize(320,240));
//    QCOMPARE(settings2.quality(), QMediaEncoderSettings:HighQuality);

//    /* Verify both the instances are equal*/
//    QCOMPARE(settings2, settings1);
//    QVERIFY(!settings2.isNull());
}

/* Test QVideoEncoderSettings Overloaded Operator assignment*/
void tst_QMediaEncoder::testVideoSettingsOperatorAssignment()
{
//    /* Create two instances.*/
//    QVideoEncoderSettings settings1;
//    QVideoEncoderSettings settings2;
//    QCOMPARE(settings2, settings1);
//    QVERIFY(settings2.isNull());

//    /* Initialize all the parameters */
//    settings1.setCodec(QLatin1String("codecName"));
//    settings1.setBitRate(128000);
//    settings1.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    settings1.setFrameRate(30000.0/10001);
//    settings1.setResolution(QSize(320,240));
//    settings1.setQuality(QMediaEncoderSettings:HighQuality);
//    /* Assign one object to other*/
//    settings2 = settings1;

//    /* Verify all the parameters are copied correctly*/
//    QCOMPARE(settings2, settings1);
//    QCOMPARE(settings2.codec(), QLatin1String("codecName"));
//    QCOMPARE(settings2.bitRate(), 128000);
//    QCOMPARE(settings2.encodingMode(), QMediaEncoderSettings:ConstantBitRateEncoding);
//    QVERIFY(qFuzzyCompare(settings2.frameRate(), qreal(30000.0/10001)));
//    QCOMPARE(settings2.resolution(), QSize(320,240));
//    QCOMPARE(settings2.quality(), QMediaEncoderSettings:HighQuality);
//    QCOMPARE(settings2, settings1);
//    QVERIFY(!settings2.isNull());
}

/* Test QVideoEncoderSettings Overloaded OperatorNotEqual*/
void tst_QMediaEncoder::testVideoSettingsOperatorNotEqual()
{
//    /* Create the instance and set the bit rate and Verify objects with OperatorNotEqual*/
//    QVideoEncoderSettings settings1;
//    settings1.setBitRate(1);
//    QVideoEncoderSettings settings2;
//    settings2.setBitRate(1);
//    /* OperatorNotEqual returns false when both objects are equal*/
//    QCOMPARE(settings1 != settings2, false);
//    settings2.setBitRate(2);
//    /* OperatorNotEqual returns true when both objects are not equal*/
//    QVERIFY(settings1 != settings2);

//    /* Verify Resolution with not equal operator*/
//    settings1 = QVideoEncoderSettings();
//    settings1.setResolution(800,600);
//    settings2 = QVideoEncoderSettings();
//    settings2.setResolution(QSize(800,600));
//    /* OperatorNotEqual returns false when both objects are equal*/
//    QCOMPARE(settings1 != settings2, false);
//    settings2.setResolution(QSize(400,300));
//    /* OperatorNotEqual returns true when both objects are not equal*/
//    QVERIFY(settings1 != settings2);

//    /* Verify Codec with not equal operator*/
//    settings1 = QVideoEncoderSettings();
//    settings1.setCodec("codec1");
//    settings2 = QVideoEncoderSettings();
//    settings2.setCodec("codec1");
//    /* OperatorNotEqual returns false when both objects are equal*/
//    QCOMPARE(settings1 != settings2, false);
//    settings2.setCodec("codec2");
//    /* OperatorNotEqual returns true when both objects are not equal*/
//    QVERIFY(settings1 != settings2);

//    /* Verify EncodingMode with not equal operator*/
//    settings1 = QVideoEncoderSettings();
//    settings1.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    settings2 = QVideoEncoderSettings();
//    settings2.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    /* OperatorNotEqual returns false when both objects are equal*/
//    QCOMPARE(settings1 != settings2, false);
//    settings2.setEncodingMode(QMediaEncoderSettings:TwoPassEncoding);
//    /* OperatorNotEqual returns true when both objects are not equal*/
//    QVERIFY(settings1 != settings2);

//    /* Verify Quality with not equal operator*/
//    settings1 = QVideoEncoderSettings();
//    settings1.setQuality(QMediaEncoderSettings:NormalQuality);
//    settings2 = QVideoEncoderSettings();
//    settings2.setQuality(QMediaEncoderSettings:NormalQuality);
//    /* OperatorNotEqual returns false when both objects are equal*/
//    QCOMPARE(settings1 != settings2, false);
//    settings2.setQuality(QMediaEncoderSettings:LowQuality);
//    /* OperatorNotEqual returns true when both objects are not equal*/
//    QVERIFY(settings1 != settings2);

//    /* Verify FrameRate with not equal operator*/
//    settings1 = QVideoEncoderSettings();
//    settings1.setFrameRate(1);
//    settings2 = QVideoEncoderSettings();
//    settings2.setFrameRate(1);
//    /* OperatorNotEqual returns false when both objects are equal*/
//    QCOMPARE(settings1 != settings2, false);
//    settings2.setFrameRate(2);
//    /* OperatorNotEqual returns true when both objects are not equal*/
//    QVERIFY(settings1 != settings2);
}

/* Test QVideoEncoderSettings Overloaded comparison operator*/
void tst_QMediaEncoder::testVideoSettingsOperatorComparison()
{
//    /* Create the instance and set the bit rate and Verify objects with comparison operator*/
//    QVideoEncoderSettings settings1;
//    settings1.setBitRate(1);
//    QVideoEncoderSettings settings2;
//    settings2.setBitRate(1);

//    /* Comparison operator returns true when both objects are equal*/
//    QVERIFY(settings1 == settings2);
//    settings2.setBitRate(2);
//    /* Comparison operator returns false when both objects are not equal*/
//    QCOMPARE(settings1 == settings2, false);

//    /* Verify resolution with comparison operator*/
//    settings1 = QVideoEncoderSettings();
//    settings1.setResolution(800,600);
//    settings2 = QVideoEncoderSettings();
//    settings2.setResolution(QSize(800,600));
//    /* Comparison operator returns true when both objects are equal*/
//    QVERIFY(settings1 == settings2);
//    settings2.setResolution(QSize(400,300));
//    /* Comparison operator returns false when both objects are not equal*/
//    QCOMPARE(settings1 == settings2, false);

//    /* Verify Codec with comparison operator*/
//    settings1 = QVideoEncoderSettings();
//    settings1.setCodec("codec1");
//    settings2 = QVideoEncoderSettings();
//    settings2.setCodec("codec1");
//    /* Comparison operator returns true when both objects are equal*/
//    QVERIFY(settings1 == settings2);
//    settings2.setCodec("codec2");
//    /* Comparison operator returns false when both objects are not equal*/
//    QCOMPARE(settings1 == settings2, false);

//    /* Verify EncodingMode with comparison operator*/
//    settings1 = QVideoEncoderSettings();
//    settings1.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    settings2 = QVideoEncoderSettings();
//    settings2.setEncodingMode(QMediaEncoderSettings:ConstantBitRateEncoding);
//    /* Comparison operator returns true when both objects are equal*/
//    QVERIFY(settings1 == settings2);
//    settings2.setEncodingMode(QMediaEncoderSettings:TwoPassEncoding);
//    /* Comparison operator returns false when both objects are not equal*/
//    QCOMPARE(settings1 == settings2, false);

//    /* Verify Quality with comparison operator*/
//    settings1 = QVideoEncoderSettings();
//    settings1.setQuality(QMediaEncoderSettings:NormalQuality);
//    settings2 = QVideoEncoderSettings();
//    settings2.setQuality(QMediaEncoderSettings:NormalQuality);
//    /* Comparison operator returns true when both objects are equal*/
//    QVERIFY(settings1 == settings2);
//    settings2.setQuality(QMediaEncoderSettings:LowQuality);
//    /* Comparison operator returns false when both objects are not equal*/
//    QCOMPARE(settings1 == settings2, false);

//    /* Verify FrameRate with comparison operator*/
//    settings1 = QVideoEncoderSettings();
//    settings1.setFrameRate(1);
//    settings2 = QVideoEncoderSettings();
//    settings2.setFrameRate(1);
//    /* Comparison operator returns true when both objects are equal*/
//    QVERIFY(settings1 == settings2);
//    settings2.setFrameRate(2);
//    /* Comparison operator returns false when both objects are not equal*/
//    QCOMPARE(settings1 == settings2, false);
}

/* Test the destuctor of the QVideoEncoderSettings*/
void tst_QMediaEncoder::testVideoSettingsDestructor()
{
//    /* Create the instance on heap and verify if object deleted correctly*/
//    QVideoEncoderSettings *settings1 = new QVideoEncoderSettings();
//    QVERIFY(settings1 != nullptr);
//    QVERIFY(settings1->isNull());
//    delete settings1;

//    /* Create the instance on heap and initialise it and verify if object deleted correctly.*/
//    QVideoEncoderSettings *settings2 = new QVideoEncoderSettings();
//    QVERIFY(settings2 != nullptr);
//    settings2->setCodec(QString("codec"));
//    QVERIFY(!settings2->isNull());
//    delete settings2;
}

QTEST_GUILESS_MAIN(tst_QMediaEncoder)
#include "tst_qmediaencoder.moc"
