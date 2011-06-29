/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "tst_qmediarecorder_xa.h"

QT_USE_NAMESPACE

void tst_QMediaRecorder_xa::initTestCase()
{
    audiosource = new QAudioCaptureSource;
    audiocapture = new QMediaRecorder(audiosource);
}

void tst_QMediaRecorder_xa::cleanupTestCase()
{
    delete audiocapture;
    delete audiosource;
}

void tst_QMediaRecorder_xa::testMediaRecorderObject()
{
    //audioocontainer types
    QCOMPARE(audiocapture->audioCodecDescription("pcm"), QString("pcm"));
    QCOMPARE(audiocapture->audioCodecDescription("amr"), QString("amr"));
    QCOMPARE(audiocapture->audioCodecDescription("aac"), QString("aac"));
    QCOMPARE(audiocapture->containerDescription("audio/wav"), QString("wav container"));
    QCOMPARE(audiocapture->containerDescription("audio/amr"), QString("amr File format"));
    QCOMPARE(audiocapture->containerDescription("audio/mpeg"), QString("mpeg container"));
    QCOMPARE(audiocapture->containerMimeType(), QString("audio/wav"));
    QCOMPARE(audiocapture->error(), QMediaRecorder::NoError);
    QCOMPARE(audiocapture->errorString(), QString());
    QCOMPARE(audiocapture->outputLocation().toLocalFile(), QString());
    QCOMPARE(audiocapture->state(), QMediaRecorder::StoppedState);
    QCOMPARE(audiocapture->supportedAudioCodecs().count(), 3); // "pcm", "amr", "aac"
    QAudioEncoderSettings settings;
    settings.setCodec("pcm");
    QCOMPARE(audiocapture->supportedAudioSampleRates(settings).count(), 5);
    bool isContinuous;
    audiocapture->supportedAudioSampleRates(settings, &isContinuous);
    QCOMPARE(isContinuous, false);
    QCOMPARE(audiocapture->supportedContainers().count(), 3); // "audio/wav", "audio/amr", "audio/mpeg"
}

void tst_QMediaRecorder_xa::testDefaultAudioEncodingSettings()
{
    QAudioEncoderSettings audioSettings = audiocapture->audioSettings();
    QCOMPARE(audioSettings.codec(), QString("pcm"));
    QCOMPARE(audiocapture->containerMimeType(), QString("audio/wav"));
    QCOMPARE(audioSettings.bitRate(), 0);
    QCOMPARE(audioSettings.channelCount(), -1);
    QCOMPARE(audioSettings.encodingMode(), QtMultimediaKit::ConstantQualityEncoding);
    QCOMPARE(audioSettings.quality(), QtMultimediaKit::NormalQuality);
    QCOMPARE(audioSettings.sampleRate(), -1);
}

void tst_QMediaRecorder_xa::testAudioRecordingLocationOnly()
{
    QSignalSpy stateSignal(audiocapture,SIGNAL(stateChanged(QMediaRecorder::State)));
    QCOMPARE(audiocapture->state(), QMediaRecorder::StoppedState);
    QTest::qWait(500);  // wait for recorder to initialize itself
    audiocapture->setOutputLocation(nextFileName(QDir::rootPath(), "locationonly", "wav"));
    audiocapture->record();
    QTRY_COMPARE(stateSignal.count(), 1); // wait for callbacks to complete in symbian API
    QCOMPARE(audiocapture->state(), QMediaRecorder::RecordingState);
    QCOMPARE(audiocapture->error(), QMediaRecorder::NoError);
    QCOMPARE(audiocapture->errorString(), QString());
    QCOMPARE(stateSignal.count(), 1);
    QTest::qWait(5000);  // wait for 5 seconds
    audiocapture->pause();
    QTRY_COMPARE(stateSignal.count(), 2); // wait for callbacks to complete in symbian API
    QCOMPARE(audiocapture->state(), QMediaRecorder::PausedState);
    QCOMPARE(stateSignal.count(), 2);
    audiocapture->stop();
    QTRY_COMPARE(stateSignal.count(), 3); // wait for callbacks to complete in symbian API
    QCOMPARE(audiocapture->state(), QMediaRecorder::StoppedState);
    QCOMPARE(stateSignal.count(), 3);
}

void tst_QMediaRecorder_xa::testAudioRecording_data()
{
    QTest::addColumn<QString>("mime"); // "audio/wav", "audio/amr", "audio/mpeg"
    QTest::addColumn<QString>("codec"); // "pcm", "amr", "aac"
    QTest::addColumn<QString>("filename_desc");
    QTest::addColumn<QString>("filename_ext"); // "wav", "amr", "mp4"
    QTest::addColumn<QString>("settings");
    QTest::addColumn<int>("bitrate");
    QTest::addColumn<int>("samplerate");
    QTest::addColumn<int>("channels");

    QTest::newRow("wav default") << "audio/wav" << "pcm" << "default" << "wav" << "default" << 0 << 0 << 0;
    QTest::newRow("amr default") << "audio/amr" << "amr" << "default" << "amr" << "default" << 4750 << 8000 << -1;
    QTest::newRow("aac default") << "audio/mpeg" << "aac" << "default" << "mp4" << "default" << 0 << 48000 << -1;
    QTest::newRow("wav 08kHz Mono") << "audio/wav" << "pcm" << "Sr08kHzMono" << "wav" << "user" << 0 << 8000 << 1;
    QTest::newRow("wav 08kHz Stereo") << "audio/wav" << "pcm" << "Sr08kHzStereo" << "wav" << "user" << 0 << 8000 << 2;
    QTest::newRow("wav 16kHz Mono") << "audio/wav" << "pcm" << "Sr16kHzMono" << "wav" << "user" << 0 << 16000 << 1;
    QTest::newRow("wav 16kHz Stereo") << "audio/wav" << "pcm" << "Sr16kHzStereo" << "wav" << "user" << 0 << 16000 << 2;
    QTest::newRow("wav 32kHz Mono") << "audio/wav" << "pcm" << "Sr32kHzMono" << "wav" << "user" << 0 << 32000 << 1;
    QTest::newRow("wav 32kHz Stereo") << "audio/wav" << "pcm" << "Sr32kHzStereo" << "wav" << "user" << 0<< 32000 << 2;
    QTest::newRow("wav 48kHz Mono") << "audio/wav" << "pcm" << "Sr48kHzMono" << "wav" << "user" << 0 << 48000 << 1;
    QTest::newRow("wav 48kHz Stereo") << "audio/wav" << "pcm" << "Sr48kHzStereo" << "wav" << "user" << 0 << 48000 << 2;
    QTest::newRow("amr Br04750bps") << "audio/amr" << "amr" << "Br04750bps" << "amr" << "user" << 4750 << 8000 << 1;
    QTest::newRow("amr Br05150bps") << "audio/amr" << "amr" << "Br05150bps" << "amr" << "user" << 5150 << 8000 << 1;
    QTest::newRow("amr Br05900bps") << "audio/amr" << "amr" << "Br05900bps" << "amr" << "user" << 5900 << 8000 << 1;
    QTest::newRow("amr Br06700bps") << "audio/amr" << "amr" << "Br06700bps" << "amr" << "user" << 6700 << 8000 << 1;
    QTest::newRow("amr Br07400bps") << "audio/amr" << "amr" << "Br07400bps" << "amr" << "user" << 7400 << 8000 << 1;
    QTest::newRow("amr Br07950bps") << "audio/amr" << "amr" << "Br07950bps" << "amr" << "user" << 7950 << 8000 << 1;
    QTest::newRow("amr Br10200bps") << "audio/amr" << "amr" << "Br10200bps" << "amr" << "user" << 10200 << 8000 << 1;
    QTest::newRow("amr Br12200bps") << "audio/amr" << "amr" << "Br12200bps" << "amr" << "user" << 10200 << 8000 << 1;
    QTest::newRow("amr verylowqual") << "audio/amr" << "amr" << "verylowqual" << "amr" << "preset" << -1 << 8000 << -1;
    QTest::newRow("amr lowqual") << "audio/amr" << "amr" << "lowqual" << "amr" << "preset" << -2 << 8000 << -1;
    QTest::newRow("amr normalqual") << "audio/amr" << "amr" << "normalqual" << "amr" << "preset" << -3 << 8000 << -1;
    QTest::newRow("amr highqual") << "audio/amr" << "amr" << "highqual" << "amr" << "preset" << -4 << 8000 << -1;
    QTest::newRow("amr veryhighqual") << "audio/amr" << "amr" << "veryhighqual" << "amr" << "preset" << -5 << 8000 << -1;

    // Combinations supported for sample rate 8kHz
    QTest::newRow("aac Br32k Sr8kHz Mono") << "audio/mpeg" << "aac" << "Br32kSr8kHzMono" << "mp4" << "user" << 32000 << 8000 << 1;
    QTest::newRow("aac Br32k Sr8kHz Stereo") << "audio/mpeg" << "aac" << "Br32kSr8kHzStereo" << "mp4" << "user" << 32000 << 8000 << 2;
    QTest::newRow("aac Br64k Sr8kHz Stereo") << "audio/mpeg" << "aac" << "Br64kSr8kHzStereo" << "mp4" << "user" << 64000 << 8000 << 2;
    // Combinations supported for sample rate 16kHz
    QTest::newRow("aac Br32k Sr16kHz Mono") << "audio/mpeg" << "aac" << "Br32kSr16kHzMono" << "mp4" << "user" << 32000 << 16000 << 1;
    QTest::newRow("aac Br64k Sr16kHz Mono") << "audio/mpeg" << "aac" << "Br64kSr16kHzMonoo" << "mp4" << "user" << 64000 << 16000 << 1;
    QTest::newRow("aac Br32k Sr16kHz Stereo") << "audio/mpeg" << "aac" << "Br32kSr16kHzStereo" << "mp4" << "user" << 32000 << 16000 << 2;
    QTest::newRow("aac Br64k Sr16kHz Stereo") << "audio/mpeg" << "aac" << "Br64kSr16kHzStereo" << "mp4" << "user" << 64000 << 16000 << 2;
    QTest::newRow("aac Br96k Sr16kHz Stereo") << "audio/mpeg" << "aac" << "Br96kSr16kHzStereo" << "mp4" << "user" << 96000 << 16000 << 2;
    QTest::newRow("aac Br128k Sr16kHz Stereo") << "audio/mpeg" << "aac" << "Br128kSr16kHzStereo" << "mp4" << "user" << 128000 << 16000 << 2;
    QTest::newRow("aac Br160k Sr16kHz Stereo") << "audio/mpeg" << "aac" << "Br160kSr16kHzStereo" << "mp4" << "user" << 160000 << 16000 << 2;
    // Combinations supported for sample rate 24kHz
    QTest::newRow("aac Br32k Sr24kHz Mono") << "audio/mpeg" << "aac" << "Br32kSr24kHzMono" << "mp4" << "user" << 32000 << 24000 << 1;
    QTest::newRow("aac Br64k Sr24kHz Mono") << "audio/mpeg" << "aac" << "Br64kSr24kHzMono" << "mp4" << "user" << 64000 << 24000 << 1;
    QTest::newRow("aac Br96k Sr24kHz Mono") << "audio/mpeg" << "aac" << "Br96kSr24kHzMono" << "mp4" << "user" << 96000 << 24000 << 1;
    QTest::newRow("aac Br32k Sr24kHz Stereo") << "audio/mpeg" << "aac" << "Br32kSr24kHzStereo" << "mp4" << "user" << 32000 << 24000 << 2;
    QTest::newRow("aac Br64k Sr24kHz Stereo") << "audio/mpeg" << "aac" << "Br64kSr24kHzStereo" << "mp4" << "user" << 64000 << 24000 << 2;
    QTest::newRow("aac Br96k Sr24kHz Stereo") << "audio/mpeg" << "aac" << "Br96kSr24kHzStereo" << "mp4" << "user" << 96000 << 24000 << 2;
    QTest::newRow("aac Br128k Sr24kHz Stereo") << "audio/mpeg" << "aac" << "Br128kSr24kHzStereo" << "mp4" << "user" << 128000 << 24000 << 2;
    QTest::newRow("aac Br160k Sr24kHz Stereo") << "audio/mpeg" << "aac" << "Br160kSr24kHzStereo" << "mp4" << "user" << 160000 << 24000 << 2;
    QTest::newRow("aac Br192k Sr24kHz Stereo") << "audio/mpeg" << "aac" << "Br192kSr24kHzStereo" << "mp4" << "user" << 192000 << 24000 << 2;
    // Combinations supported for sample rate 32kHz
    QTest::newRow("aac Br32k Sr32kHz Mono") << "audio/mpeg" << "aac" << "Br32kSr32kHzMono" << "mp4" << "user" << 32000 << 32000 << 1;
    QTest::newRow("aac Br64k Sr32kHz Mono") << "audio/mpeg" << "aac" << "Br64kSr32kHzMono" << "mp4" << "user" << 64000 << 32000 << 1;
    QTest::newRow("aac Br96k Sr32kHz Mono") << "audio/mpeg" << "aac" << "Br96kSr32kHzMono" << "mp4" << "user" << 96000 << 32000 << 1;
    QTest::newRow("aac Br128k Sr32kHz Mono") << "audio/mpeg" << "aac" << "Br128kSr32kHzMono" << "mp4" << "user" << 128000 << 32000 << 1;
    QTest::newRow("aac Br160k Sr32kHz Mono") << "audio/mpeg" << "aac" << "Br160kSr32kHzMono" << "mp4" << "user" << 160000 << 32000 << 1;
    QTest::newRow("aac Br32k Sr32kHz Stereo") << "audio/mpeg" << "aac" << "Br32kSr32kHzStereo" << "mp4" << "user" << 32000 << 32000 << 2;
    QTest::newRow("aac Br64k Sr32kHz Stereo") << "audio/mpeg" << "aac" << "Br64kSr32kHzStereo" << "mp4" << "user" << 64000 << 32000 << 2;
    QTest::newRow("aac Br96k Sr32kHz Stereo") << "audio/mpeg" << "aac" << "Br96kSr32kHzStereo" << "mp4" << "user" << 96000 << 32000 << 2;
    QTest::newRow("aac Br128k Sr32kHz Stereo") << "audio/mpeg" << "aac" << "Br128kSr32kHzStereo" << "mp4" << "user" << 128000 << 32000 << 2;
    QTest::newRow("aac Br160k Sr32kHz Stereo") << "audio/mpeg" << "aac" << "Br160kSr32kHzStereo" << "mp4" << "user" << 160000 << 32000 << 2;
    QTest::newRow("aac Br192k Sr32kHz Stereo") << "audio/mpeg" << "aac" << "Br192kSr32kHzStereo" << "mp4" << "user" << 192000 << 32000 << 2;
    QTest::newRow("aac Br224k Sr32kHz Stereo") << "audio/mpeg" << "aac" << "Br224kSr32kHzStereo" << "mp4" << "user" << 224000 << 32000 << 2;
    QTest::newRow("aac Br256k Sr32kHz Stereo") << "audio/mpeg" << "aac" << "Br256kSr32kHzStereo" << "mp4" << "user" << 256000 << 32000 << 2;
    // Combinations supported for sample rate 48kHz
    QTest::newRow("aac Br32k Sr48kHz Mono") << "audio/mpeg" << "aac" << "Br32kSr48kHzMono" << "mp4" << "user" << 32000 << 48000 << 1;
    QTest::newRow("aac Br64k Sr48kHz Mono") << "audio/mpeg" << "aac" << "Br64kSr48kHzMono" << "mp4" << "user" << 64000 << 48000 << 1;
    QTest::newRow("aac Br96k Sr48kHz Mono") << "audio/mpeg" << "aac" << "Br96kSr48kHzMono" << "mp4" << "user" << 96000 << 48000 << 1;
    QTest::newRow("aac Br128k Sr48kHz Mono") << "audio/mpeg" << "aac" << "Br128kSr48kHzMono" << "mp4" << "user" << 128000 << 48000 << 1;
    QTest::newRow("aac Br160k Sr48kHz Mono") << "audio/mpeg" << "aac" << "Br160kSr48kHzMono" << "mp4" << "user" << 160000 << 48000 << 1;
    QTest::newRow("aac Br192k Sr48kHz Mono") << "audio/mpeg" << "aac" << "Br192kSr48kHzMono" << "mp4" << "user" << 192000 << 48000 << 1;
    QTest::newRow("aac Br224k Sr48kHz Mono") << "audio/mpeg" << "aac" << "Br224kSr48kHzMono" << "mp4" << "user" << 224000 << 48000 << 1;
    QTest::newRow("aac Br32k Sr48kHz Stereo") << "audio/mpeg" << "aac" << "Br32kSr48kHzStereo" << "mp4" << "user" << 32000 << 48000 << 2;
    QTest::newRow("aac Br64k Sr48kHz Stereo") << "audio/mpeg" << "aac" << "Br64kSr48kHzStereo" << "mp4" << "user" << 64000 << 48000 << 2;
    QTest::newRow("aac Br96k Sr48kHz Stereo") << "audio/mpeg" << "aac" << "Br96kSr48kHzStereo" << "mp4" << "user" << 96000 << 48000 << 2;
    QTest::newRow("aac Br128k Sr48kHz Stereo") << "audio/mpeg" << "aac" << "Br128kSr48kHzStereo" << "mp4" << "user" << 128000 << 48000 << 2;
    QTest::newRow("aac Br160k Sr48kHz Stereo") << "audio/mpeg" << "aac" << "Br160kSr48kHzStereo" << "mp4" << "user" << 160000 << 48000 << 2;
    QTest::newRow("aac Br192k Sr48kHz Stereo") << "audio/mpeg" << "aac" << "Br192kSr48kHzStereo" << "mp4" << "user" << 192000 << 48000 << 2;
    QTest::newRow("aac Br224k Sr48kHz Stereo") << "audio/mpeg" << "aac" << "Br224kSr48kHzStereo" << "mp4" << "user" << 224000 << 48000 << 2;
    QTest::newRow("aac Br256k Sr48kHz Stereo") << "audio/mpeg" << "aac" << "Br256kSr48kHzStereo" << "mp4" << "user" << 256000 << 48000 << 2;
    QTest::newRow("aac verylowqual") << "audio/mpeg" << "aac" << "verylowqual" << "mp4" << "preset" << -1 << 8000 << -1;
    QTest::newRow("aac lowqual") << "audio/mpeg" << "aac" << "lowqual" << "mp4" << "preset" << -2 << 24000 << -1;
    QTest::newRow("aac normalqual") << "audio/mpeg" << "aac" << "normalqual" << "mp4" << "preset" << -3 << 32000 << -1;
    QTest::newRow("aac highqual") << "audio/mpeg" << "aac" << "highqual" << "mp4" << "preset" << -4 << 48000 << -1;
    QTest::newRow("aac veryhighqual") << "audio/mpeg" << "aac" << "veryhighqual" << "mp4" << "preset" << -5 << 48000 << -1;
    }

void tst_QMediaRecorder_xa::testAudioRecording()
{
    QFETCH(QString, mime);
    QFETCH(QString, codec);
    QFETCH(QString, filename_desc);
    QFETCH(QString, filename_ext);
    QFETCH(QString, settings);

    QSignalSpy stateSignal(audiocapture,SIGNAL(stateChanged(QMediaRecorder::State)));
    audiocapture->setOutputLocation(nextFileName(QDir::rootPath(), filename_desc, filename_ext));
    QAudioEncoderSettings audioSettings;
    audioSettings.setCodec(codec);
    if (settings.compare("default") == 0) {
        audioSettings.setSampleRate(-1);
    }
    else if (settings.compare("user") == 0) {
        QFETCH(int, bitrate);
        QFETCH(int, samplerate);
        QFETCH(int, channels);
        audioSettings.setEncodingMode(QtMultimediaKit::ConstantBitRateEncoding);
        if (bitrate > 0)
            audioSettings.setBitRate(bitrate);
        if (samplerate > 0)
            audioSettings.setSampleRate(samplerate);
        if ((channels > 0) || (channels == -1))
            audioSettings.setChannelCount(channels);
    }
    else if (settings.compare("preset") == 0) {
        QFETCH(int, bitrate);
        QFETCH(int, samplerate);
        QFETCH(int, channels);
        audioSettings.setEncodingMode(QtMultimediaKit::ConstantQualityEncoding);
        QtMultimediaKit::EncodingQuality quality = QtMultimediaKit::NormalQuality;
        switch(bitrate) {
            case -1:
                quality = QtMultimediaKit::VeryLowQuality;
                break;
            case -2:
                quality = QtMultimediaKit::LowQuality;
                break;
            default:
            case -3:
                quality = QtMultimediaKit::NormalQuality;
                break;
            case -4:
                quality = QtMultimediaKit::HighQuality;
                break;
            case -5:
                quality = QtMultimediaKit::VeryHighQuality;
                break;
        }
        audioSettings.setQuality(quality);
        if (samplerate > 0)
            audioSettings.setSampleRate(samplerate);
        if ((channels > 0) || (channels == -1))
            audioSettings.setChannelCount(channels);
    }
    QVideoEncoderSettings videoSettings;
    audiocapture->setEncodingSettings(audioSettings, videoSettings, mime);
    QCOMPARE(audiocapture->state(), QMediaRecorder::StoppedState);
    QTest::qWait(500);  // wait for recorder to initialize itself
    int expectedSignalCount = 1;
    audiocapture->record();
    QTRY_COMPARE(stateSignal.count(), expectedSignalCount); // wait for callbacks to complete in symbian API
    QCOMPARE(audiocapture->state(), QMediaRecorder::RecordingState);
    QCOMPARE(audiocapture->error(), QMediaRecorder::NoError);
    QCOMPARE(audiocapture->errorString(), QString());
    QCOMPARE(stateSignal.count(), expectedSignalCount);
    QTest::qWait(5000);  // wait for 5 seconds
    // If we are not testing aac recording to mp4 container
    if (!((mime.compare("audio/mpeg") == 0) && (codec.compare("aac") == 0))) {
        expectedSignalCount++;
        audiocapture->pause();
        QTRY_COMPARE(stateSignal.count(), expectedSignalCount); // wait for callbacks to complete in symbian API
        QCOMPARE(audiocapture->state(), QMediaRecorder::PausedState);
        QCOMPARE(stateSignal.count(), expectedSignalCount);
    }
    expectedSignalCount++;
    audiocapture->stop();
    QTRY_COMPARE(stateSignal.count(), expectedSignalCount); // wait for callbacks to complete in symbian API
    QCOMPARE(audiocapture->state(), QMediaRecorder::StoppedState);
    QCOMPARE(stateSignal.count(), expectedSignalCount);
    /* testAudioRecording() function gets executed for each rown in the table.
     * If and when all tests in the table passes, test log will just contain one
     * entry 'PASS   : tst_QMediaRecorder_xa::testAudioRecording()'. To figure out
     * which test in the loop completed successfully, just print a debug message
     * which also goes into the test log generated.*/
    qDebug() << "----> PASS";
}

void tst_QMediaRecorder_xa::testOutputLocation()
{
    audiocapture->setOutputLocation(QUrl("test.wav"));
    QUrl s = audiocapture->outputLocation();
    QCOMPARE(s.toString(), QString("test.wav"));
}

QUrl tst_QMediaRecorder_xa::nextFileName(QDir outputDir, QString appendName, QString ext)
{
    int lastImage = 0;
    int fileCount = 0;
    foreach( QString fileName, outputDir.entryList(QStringList() << "testclip_*." + ext) ) {
        int imgNumber = fileName.mid(5, fileName.size()-9).toInt();
        lastImage = qMax(lastImage, imgNumber);
        if (outputDir.exists(fileName))
            fileCount+=1;
    }
    lastImage+=fileCount;

    QUrl location(QDir::toNativeSeparators(outputDir.canonicalPath() + QString("/testclip_%1").arg(lastImage+1 , 4, 10, QLatin1Char('0')) + appendName + "." + ext));
    return location;
}
