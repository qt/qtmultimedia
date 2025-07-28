// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>
#include <QtTest/qtesteventloop.h>
#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/qsoundeffect.h>

#include <QtMultimedia/private/qsoundeffectsynchronous_p.h>

using namespace Qt::Literals;

class tst_QSoundEffect : public QObject
{
    Q_OBJECT
public:
    tst_QSoundEffect(QObject* parent=nullptr) : QObject(parent) {}

public slots:
    void init();
    void cleanup();

private slots:
    void initTestCase();
    void testSource();
    void testLooping();
    void testVolume();
    void testMuting();

    void testPlaying();
    void testStatus();
    void loopsRemaining_isUpdatedWhenPlaying();

    void testDestroyWhilePlaying();
    void testDestroyWhileRestartPlaying();

    void testSetSourceWhileLoading();
    void testSetSourceWhilePlaying();
    void testSupportedMimeTypes_data();
    void testSupportedMimeTypes();
    void testCorruptFile();

    void setAudioDevice_emitsSignalsInExpectedOrder_data();
    void setAudioDevice_emitsSignalsInExpectedOrder();

private:
    QSoundEffect* sound;
    QUrl url; // test.wav: pcm_s16le, 48000 Hz, stereo, s16
    QUrl url2; // test_tone.wav: pcm_s16le, 44100 Hz, mono
    QUrl urlCorrupted; // test_corrupted.wav: corrupted
};

void tst_QSoundEffect::init()
{
    sound = new QSoundEffect(this);
}

void tst_QSoundEffect::cleanup()
{
    sound = nullptr;
}

void tst_QSoundEffect::initTestCase()
{
    // Only perform tests if audio device exists
    QStringList mimeTypes = sound->supportedMimeTypes();
    if (mimeTypes.empty())
        QSKIP("No audio devices available");

    QString testFileName = QStringLiteral("test.wav");
    QString fullPath = QFINDTESTDATA(testFileName);
    QVERIFY2(!fullPath.isEmpty(), qPrintable(QStringLiteral("Unable to locate ") + testFileName));
    url = QUrl::fromLocalFile(fullPath);

    testFileName = QStringLiteral("test_tone.wav");
    fullPath = QFINDTESTDATA(testFileName);
    QVERIFY2(!fullPath.isEmpty(), qPrintable(QStringLiteral("Unable to locate ") + testFileName));
    url2 = QUrl::fromLocalFile(fullPath);

    testFileName = QStringLiteral("test_corrupted.wav");
    fullPath = QFINDTESTDATA(testFileName);
    QVERIFY2(!fullPath.isEmpty(), qPrintable(QStringLiteral("Unable to locate ") + testFileName));
    urlCorrupted = QUrl::fromLocalFile(fullPath);

    sound = new QSoundEffect(this);

    QVERIFY(sound->source().isEmpty());
    QVERIFY(sound->loopCount() == 1);
    QVERIFY(sound->volume() == 1);
    QVERIFY(sound->isMuted() == false);
    sound = nullptr;
}

void tst_QSoundEffect::testSource()
{
    QSignalSpy readSignal(sound, &QSoundEffect::sourceChanged);

    sound->setSource(url);
    sound->setVolume(0.1f);

    QCOMPARE(sound->source(),url);
    QCOMPARE(readSignal.size(),1);

    QTestEventLoop::instance().enterLoop(1);
    sound->play();
    QTRY_COMPARE(sound->isPlaying(), false);
    QCOMPARE(sound->loopsRemaining(), 0);
}

void tst_QSoundEffect::testLooping()
{
    sound->setSource(url);
    QTRY_COMPARE(sound->status(), QSoundEffect::Ready);

    QSignalSpy readSignal_Count(sound, &QSoundEffect::loopCountChanged);
    QSignalSpy readSignal_Remaining(sound, &QSoundEffect::loopsRemainingChanged);

    sound->setLoopCount(3);
    sound->setVolume(0.1f);
    QCOMPARE(sound->loopCount(), 3);
    QCOMPARE(readSignal_Count.size(), 1);
    QCOMPARE(sound->loopsRemaining(), 0);
    QCOMPARE(readSignal_Remaining.size(), 0);

    sound->play();
    QVERIFY(readSignal_Remaining.size() > 0);

    // test.wav is about 200ms, wait until it has finished playing 3 times
    QTestEventLoop::instance().enterLoop(3);

    QTRY_COMPARE(sound->loopsRemaining(), 0);
    QCOMPARE(readSignal_Remaining.size(), 4);
    QTRY_VERIFY(!sound->isPlaying());

    // QTBUG-36643 (setting the loop count while playing should work)
    {
        readSignal_Count.clear();
        readSignal_Remaining.clear();

        sound->setLoopCount(10);
        QCOMPARE(sound->loopCount(), 10);
        QCOMPARE(readSignal_Count.size(), 1);
        QCOMPARE(sound->loopsRemaining(), 0);
        QCOMPARE(readSignal_Remaining.size(), 0);

        sound->play();
        QVERIFY(readSignal_Remaining.size() > 0);

        // wait for the sound to be played several times
        QTRY_VERIFY(sound->loopsRemaining() <= 7);
        QVERIFY(readSignal_Remaining.size() >= 3);
        readSignal_Count.clear();
        readSignal_Remaining.clear();

        // change the loop count while playing
        sound->setLoopCount(3);
        QCOMPARE(sound->loopCount(), 3);
        QCOMPARE(readSignal_Count.size(), 1);
        QCOMPARE(sound->loopsRemaining(), 3);
        QCOMPARE(readSignal_Remaining.size(), 1);

        // wait for all the loops to be completed
        QTRY_COMPARE(sound->loopsRemaining(), 0);
        QTRY_VERIFY(readSignal_Remaining.size() == 4);
        QTRY_VERIFY(!sound->isPlaying());
    }

    {
        readSignal_Count.clear();
        readSignal_Remaining.clear();

        sound->setLoopCount(QSoundEffect::Infinite);
        QCOMPARE(sound->loopCount(), int(QSoundEffect::Infinite));
        QCOMPARE(readSignal_Count.size(), 1);
        QCOMPARE(sound->loopsRemaining(), 0);
        QCOMPARE(readSignal_Remaining.size(), 0);

        sound->play();
        QTRY_COMPARE(sound->loopsRemaining(), int(QSoundEffect::Infinite));
        QCOMPARE(readSignal_Remaining.size(), 1);

        QTest::qWait(500);
        QVERIFY(sound->isPlaying());
        readSignal_Count.clear();
        readSignal_Remaining.clear();

        // Setting the loop count to 0 should play it one last time
        sound->setLoopCount(0);
        QCOMPARE(sound->loopCount(), 1);
        QCOMPARE(readSignal_Count.size(), 1);
        QCOMPARE(sound->loopsRemaining(), 1);
        QCOMPARE(readSignal_Remaining.size(), 1);

        QTRY_COMPARE(sound->loopsRemaining(), 0);
        QTRY_VERIFY(readSignal_Remaining.size() >= 2);
        QTRY_VERIFY(!sound->isPlaying());
    }
}

void tst_QSoundEffect::testVolume()
{
    QSignalSpy readSignal(sound, &QSoundEffect::volumeChanged);

    sound->setVolume(0.5);
    QCOMPARE(sound->volume(),0.5);

    QTRY_COMPARE(readSignal.size(),1);
}

void tst_QSoundEffect::testMuting()
{
    QSignalSpy readSignal(sound, &QSoundEffect::mutedChanged);

    sound->setMuted(true);
    QCOMPARE(sound->isMuted(),true);

    QTRY_COMPARE(readSignal.size(),1);
}

void tst_QSoundEffect::testPlaying()
{
    QSignalSpy playingChangedSignal(sound, &QSoundEffect::playingChanged);

    sound->setLoopCount(QSoundEffect::Infinite);
    sound->setVolume(0.1f);
    //valid source
    sound->setSource(url);
    QTestEventLoop::instance().enterLoop(1);
    sound->play();
    QTestEventLoop::instance().enterLoop(1);
    QTRY_COMPARE(sound->isPlaying(), true);
    QCOMPARE(playingChangedSignal.size(), 1);
    sound->stop();
    QCOMPARE(playingChangedSignal.size(), 2);

    //empty source
    playingChangedSignal.clear();
    sound->setSource(QUrl());
    QTestEventLoop::instance().enterLoop(1);
    sound->play();
    QTestEventLoop::instance().enterLoop(1);
    QTRY_COMPARE(sound->isPlaying(), false);
    QCOMPARE(playingChangedSignal.size(), 0);

    //invalid source
    sound->setSource(QUrl((QLatin1String("invalid source"))));
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, QRegularExpression(".*Error decoding source.*"));
    QTestEventLoop::instance().enterLoop(1);
    sound->play();
    QTestEventLoop::instance().enterLoop(1);
    QTRY_COMPARE(sound->isPlaying(), false);

    sound->setLoopCount(1); // TODO: What if one of the tests fail?
}

void tst_QSoundEffect::testStatus()
{
    sound->setSource(QUrl());
    QCOMPARE(sound->status(), QSoundEffect::Null);

    //valid source
    sound->setSource(url);

    QTestEventLoop::instance().enterLoop(1);
    QCOMPARE(sound->status(), QSoundEffect::Ready);

    //empty source
    sound->setSource(QUrl());
    QTestEventLoop::instance().enterLoop(1);
    QCOMPARE(sound->status(), QSoundEffect::Null);

    //invalid source
    sound->setLoopCount(QSoundEffect::Infinite);

    sound->setSource(QUrl(QLatin1String("invalid source")));
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, QRegularExpression(".*Error decoding source.*"));
    QTestEventLoop::instance().enterLoop(1);
    QCOMPARE(sound->status(), QSoundEffect::Error);
}

void tst_QSoundEffect::loopsRemaining_isUpdatedWhenPlaying()
{
    sound->setSource(url);
    QTRY_COMPARE(sound->status(), QSoundEffect::Ready);
    sound->setLoopCount(3);

    QSignalSpy loopsRemaining(sound, &QSoundEffect::loopsRemainingChanged);
    QCOMPARE(sound->loopsRemaining(), 0);
    sound->play();

    QTRY_COMPARE(sound->loopsRemaining(), 0);
    QCOMPARE(loopsRemaining.size(), 4);
}

void tst_QSoundEffect::testDestroyWhilePlaying()
{
    QSoundEffect *instance = new QSoundEffect();
    instance->setSource(url);
    instance->setVolume(0.1f);
    QTestEventLoop::instance().enterLoop(1);
    instance->play();
    QTest::qWait(100);
    delete instance;
    QTestEventLoop::instance().enterLoop(1);
}

void tst_QSoundEffect::testDestroyWhileRestartPlaying()
{
    QSoundEffect *instance = new QSoundEffect();
    instance->setSource(url);
    instance->setVolume(0.1f);
    QTestEventLoop::instance().enterLoop(1);
    instance->play();
    QTRY_COMPARE(instance->isPlaying(), false);
    //restart playing
    instance->play();
    delete instance;
    QTestEventLoop::instance().enterLoop(1);
}

void tst_QSoundEffect::testSetSourceWhileLoading()
{
    for (int i = 0; i < 2; i++) {
        sound->setSource(url);
        QVERIFY(sound->status() == QSoundEffect::Loading || sound->status() == QSoundEffect::Ready);
        sound->setSource(url); // set same source again
        QVERIFY(sound->status() == QSoundEffect::Loading || sound->status() == QSoundEffect::Ready);
        QTRY_COMPARE(sound->status(), QSoundEffect::Ready); // make sure it switches to ready state
        sound->play();
        QVERIFY(sound->isPlaying());

        sound->setSource(QUrl());
        QCOMPARE(sound->status(), QSoundEffect::Null);

        sound->setSource(url2);
        QVERIFY(sound->status() == QSoundEffect::Loading || sound->status() == QSoundEffect::Ready);
        sound->setSource(url); // set different source
        QVERIFY(sound->status() == QSoundEffect::Loading || sound->status() == QSoundEffect::Ready);
        QTRY_COMPARE(sound->status(), QSoundEffect::Ready);
        sound->play();
        QVERIFY(sound->isPlaying());
        sound->stop();

        sound->setSource(QUrl());
        QCOMPARE(sound->status(), QSoundEffect::Null);
    }
}

void tst_QSoundEffect::testSetSourceWhilePlaying()
{
    for (int i = 0; i < 2; i++) {
        sound->setSource(url);
        QTRY_COMPARE(sound->status(), QSoundEffect::Ready);
        sound->play();
        QVERIFY(sound->isPlaying());
        sound->setSource(url); // set same source again
        QCOMPARE(sound->status(), QSoundEffect::Ready);
        QVERIFY(sound->isPlaying()); // playback doesn't stop, URL is the same
        sound->play();
        QVERIFY(sound->isPlaying());

        sound->setSource(QUrl());
        QCOMPARE(sound->status(), QSoundEffect::Null);

        sound->setSource(url2);
        QTRY_COMPARE(sound->status(), QSoundEffect::Ready);
        sound->play();
        QVERIFY(sound->isPlaying());
        sound->setSource(url); // set different source
        QTRY_COMPARE(sound->status(), QSoundEffect::Ready);
        QVERIFY(!sound->isPlaying()); // playback stops, URL is different
        sound->play();
        QVERIFY(sound->isPlaying());
        sound->stop();

        sound->setSource(QUrl());
        QCOMPARE(sound->status(), QSoundEffect::Null);
    }
}

void tst_QSoundEffect::testSupportedMimeTypes_data()
{
    // Verify also passing of audio device info as parameter
    QTest::addColumn<QSoundEffect*>("instance");
    QTest::newRow("without QAudioDevice") << sound;
    QAudioDevice deviceInfo(QMediaDevices::defaultAudioOutput());
    QTest::newRow("with QAudioDevice")    << new QSoundEffect(deviceInfo, this);
}

void tst_QSoundEffect::testSupportedMimeTypes()
{
    QFETCH(QSoundEffect*, instance);
    QStringList mimeTypes = instance->supportedMimeTypes();
    QVERIFY(!mimeTypes.empty());
    QVERIFY(mimeTypes.indexOf(QLatin1String("audio/wav")) != -1 ||
            mimeTypes.indexOf(QLatin1String("audio/x-wav")) != -1 ||
            mimeTypes.indexOf(QLatin1String("audio/wave")) != -1 ||
            mimeTypes.indexOf(QLatin1String("audio/x-pn-wav")) != -1);
}

void tst_QSoundEffect::testCorruptFile()
{
    using namespace Qt::Literals;
    auto expectedMessagePattern =
            QRegularExpression(uR"(^QSoundEffect.*: Error decoding source .*$)"_s);

    for (int i = 0; i < 10; i++) {
        QSignalSpy statusSpy(sound, &QSoundEffect::statusChanged);
        QTest::ignoreMessage(QtMsgType::QtWarningMsg, expectedMessagePattern);

        sound->setSource(urlCorrupted);
        QVERIFY(!sound->isPlaying());
        QVERIFY(sound->status() == QSoundEffect::Loading || sound->status() == QSoundEffect::Error);
        QTRY_COMPARE(sound->status(), QSoundEffect::Error);
        QCOMPARE(statusSpy.size(), 2);
        sound->play();
        QVERIFY(!sound->isPlaying());

        sound->setSource(url);
        QTRY_COMPARE(sound->status(), QSoundEffect::Ready);
        sound->play();
        QVERIFY(sound->isPlaying());
    }
}

void tst_QSoundEffect::setAudioDevice_emitsSignalsInExpectedOrder_data()
{
    QTest::addColumn<bool>("while_playing");
    QTest::addColumn<bool>("with_source");
    QTest::addColumn<QStringList>("expectedSignals");
    QSoundEffect sfx(this);

    if (dynamic_cast<QSoundEffectPrivateSynchronous *>(QSoundEffectPrivate::get(&sfx))) {
        QTest::addRow("while_playing")
                << true << true
                << QStringList{ u"playingChanged"_s, u"playingChanged"_s, u"audioDeviceChanged"_s };
    } else {
        QTest::addRow("while_playing")
                << true << true << QStringList{ u"playingChanged"_s, u"audioDeviceChanged"_s };
    }
    QTest::addRow("while_stopped, with source")
            << false << true << QStringList{ u"audioDeviceChanged"_s };
    QTest::addRow("while_stopped, without source")
            << false << false << QStringList{ u"audioDeviceChanged"_s };
}

void tst_QSoundEffect::setAudioDevice_emitsSignalsInExpectedOrder()
{
    // Arrange
    QList outputs = QMediaDevices::audioOutputs();
    if (outputs.size() < 2)
        QSKIP("Needs at least 2 audioOuputs");

    QFETCH(bool, while_playing);
    QFETCH(bool, with_source);
    QFETCH(QStringList, expectedSignals);

    // Track the order of emitted signals by appending them to a list
    QStringList emittedSignals;
    connect(sound, &QSoundEffect::audioDeviceChanged, this, [&emittedSignals]() {
        emittedSignals.append(u"audioDeviceChanged"_s);
    });
    connect(sound, &QSoundEffect::playingChanged, this, [&emittedSignals]() {
        emittedSignals.append(u"playingChanged"_s);
    });
    connect(sound, &QSoundEffect::statusChanged, this, [&emittedSignals]() {
        emittedSignals.append(u"statusChanged"_s);
    });

    // Set source or not based on test data
    if (with_source)
        sound->setSource(url2); // Long tone to prevent playback to stop before test finishes
    QTRY_COMPARE(sound->isLoaded(), with_source);

    // Start playback or not based on test data
    if (while_playing) {
        sound->play();
    }
    QTRY_COMPARE(sound->isPlaying(), while_playing);

    QAudioDevice nonDefaultDevice = [&] {
        if (!outputs[0].isDefault())
            return outputs[0];
        return outputs[1];
    }();

    QVERIFY(nonDefaultDevice != outputs.at(1));
    emittedSignals.clear();

    // Act
    sound->setAudioDevice(nonDefaultDevice);

    // Assert
    QTRY_VERIFY(sound->isPlaying() == while_playing); // Verify that playback state didn't change
    QCOMPARE(emittedSignals, expectedSignals);
}

QTEST_MAIN(tst_QSoundEffect)

#include "tst_qsoundeffect.moc"
