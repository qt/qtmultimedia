// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>
#include <QtTest/qtesteventloop.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qmediadevices.h>

#include <QtCore/qglobal.h>

#include <QtSpatialAudio/qspatialsound.h>
#include <QtSpatialAudio/qaudioengine.h>
#include <QtSpatialAudio/qaudioroom.h>
#include <QtSpatialAudio/qaudiolistener.h>
#include <QtSpatialAudio/qambientsound.h>

#include <memory>

using namespace Qt::Literals;

class tst_QSpatialAudio : public QObject
{
    Q_OBJECT
public:
    tst_QSpatialAudio(QObject* parent=nullptr) : QObject(parent) {}

private slots:
    void init();
    void cleanup();

    void initTestCase();
    void test_QSpatialSound_basicProperties();
    void test_QSpatialSound_rotation_and_directivityOrder_semantics();
    void test_engine_positionDistanceScale();
    void test_QSpatialSound_signalsEmittedOnChange();
    void testEngineIntegration();
    void testSetSource_autoPlay();
    void testQAmbientSound_basic();
    void testQAudioRoom_basic();
    void testQAudioListener_basic();
    void testSwitchAudioDevice();

private:
    std::unique_ptr<QSpatialSound> sound;
    std::unique_ptr<QAudioEngine> engine;
    QUrl url;
};

void tst_QSpatialAudio::init()
{
    engine = std::make_unique<QAudioEngine>();
    sound = std::make_unique<QSpatialSound>(engine.get());
}

void tst_QSpatialAudio::cleanup()
{
    sound.reset();
    engine.reset();
}

void tst_QSpatialAudio::initTestCase()
{
    // locate test audio file but do not fail tests if not present; playback tests will skip
    QString testFileName = QStringLiteral("test.wav");
    QString fullPath = QFINDTESTDATA(testFileName);
    if (!fullPath.isEmpty())
        url = QUrl::fromLocalFile(fullPath);
}

void tst_QSpatialAudio::test_QSpatialSound_basicProperties()
{
    // verify defaults and simple setters/getters
    QCOMPARE(sound->loops(), 1);
    QCOMPARE(sound->autoPlay(), true);

    QSignalSpy loopsSpy(sound.get(), &QSpatialSound::loopsChanged);
    sound->setLoops(QSpatialSound::Infinite);
    QCOMPARE(sound->loops(), -1);
    QCOMPARE(loopsSpy.size(), 1);
    sound->setLoops(1);
    QCOMPARE(loopsSpy.size(), 2);

    QSignalSpy autoPlaySpy(sound.get(), &QSpatialSound::autoPlayChanged);
    sound->setAutoPlay(false);
    QCOMPARE(sound->autoPlay(), false);
    QCOMPARE(autoPlaySpy.size(), 1);
    sound->setAutoPlay(true);
    QCOMPARE(autoPlaySpy.size(), 2);

    QSignalSpy volumeSpy(sound.get(), &QSpatialSound::volumeChanged);
    sound->setVolume(0.5f);
    QCOMPARE(sound->volume(), 0.5f);
    QCOMPARE(volumeSpy.size(), 1);

    QSignalSpy directivitySpy(sound.get(), &QSpatialSound::directivityChanged);
    sound->setDirectivity(0.2f);
    QCOMPARE(sound->directivity(), 0.2f);
    QCOMPARE(directivitySpy.size(), 1);

    QSignalSpy directivityOrderSpy(sound.get(), &QSpatialSound::directivityOrderChanged);
    sound->setDirectivityOrder(2.0f);
    QCOMPARE(sound->directivityOrder(), 2.0f);
    QCOMPARE(directivityOrderSpy.size(), 1);

    QSignalSpy nfSpy(sound.get(), &QSpatialSound::nearFieldGainChanged);
    sound->setNearFieldGain(0.3f);
    QCOMPARE(sound->nearFieldGain(), 0.3f);
    QCOMPARE(nfSpy.size(), 1);

    QSignalSpy manualSpy(sound.get(), &QSpatialSound::manualAttenuationChanged);
    sound->setManualAttenuation(0.7f);
    QCOMPARE(sound->manualAttenuation(), 0.7f);
    QCOMPARE(manualSpy.size(), 1);
}

void tst_QSpatialAudio::test_engine_positionDistanceScale()
{
    // choose meter scale
    engine->setDistanceScale(QAudioEngine::DistanceScaleMeter);

    QVector3D pos(1.0f, 2.0f, 3.0f);
    QSignalSpy spy(sound.get(), &QSpatialSound::positionChanged);
    sound->setPosition(pos);
    QCOMPARE(sound->position(), pos);
    QCOMPARE(spy.size(), 1);

    const int before = spy.size();
    sound->setPosition(pos);
    QCOMPARE(spy.size(), before);
}

void tst_QSpatialAudio::test_QSpatialSound_signalsEmittedOnChange()
{
    QSignalSpy vspy(sound.get(), &QSpatialSound::volumeChanged);
    sound->setVolume(0.25f);
    QCOMPARE(vspy.size(), 1);
    sound->setVolume(0.25f);
    QCOMPARE(vspy.size(), 1);

    QSignalSpy dspy(sound.get(), &QSpatialSound::distanceModelChanged);
    sound->setDistanceModel(QSpatialSound::DistanceModel::Linear);
    QCOMPARE(dspy.size(), 1);
    sound->setDistanceModel(QSpatialSound::DistanceModel::Linear);
    QCOMPARE(dspy.size(), 1);

    QSignalSpy rotationSpy(sound.get(), &QSpatialSound::rotationChanged);
    QQuaternion q(1, 1, 0, 0);
    sound->setRotation(q);
    QCOMPARE(rotationSpy.size(), 1);
    sound->setRotation(q);
    QCOMPARE(rotationSpy.size(), 1);

    // Verify other property signals follow the expected "emit on change" semantics.
    QSignalSpy loopsSpy(sound.get(), &QSpatialSound::loopsChanged);
    sound->setLoops(QSpatialSound::Infinite);
    QCOMPARE(loopsSpy.size(), 1);
    sound->setLoops(QSpatialSound::Infinite);
    QCOMPARE(loopsSpy.size(), 1);

    QSignalSpy autoPlaySpy(sound.get(), &QSpatialSound::autoPlayChanged);
    sound->setAutoPlay(false);
    QCOMPARE(autoPlaySpy.size(), 1);
    sound->setAutoPlay(false);
    QCOMPARE(autoPlaySpy.size(), 1);

    // size and distanceCutoff use the engine's distanceScale; ensure an
    // engine is present before exercising those properties.
    sound = {};
    engine = std::make_unique<QAudioEngine>();
    sound = std::make_unique<QSpatialSound>(engine.get());

    QSignalSpy sizeSpy(sound.get(), &QSpatialSound::sizeChanged);
    sound->setSize(2.5f);
    QCOMPARE(sizeSpy.size(), 1);
    sound->setSize(2.5f);
    QCOMPARE(sizeSpy.size(), 1);

    QSignalSpy cutoffSpy(sound.get(), &QSpatialSound::distanceCutoffChanged);
    sound->setDistanceCutoff(3.5f);
    QCOMPARE(cutoffSpy.size(), 1);
    sound->setDistanceCutoff(3.5f);
    QCOMPARE(cutoffSpy.size(), 1);

    QSignalSpy manualSpy(sound.get(), &QSpatialSound::manualAttenuationChanged);
    sound->setManualAttenuation(0.6f);
    QCOMPARE(manualSpy.size(), 1);
    sound->setManualAttenuation(0.6f);
    QCOMPARE(manualSpy.size(), 1);

    QSignalSpy occSpy(sound.get(), &QSpatialSound::occlusionIntensityChanged);
    sound->setOcclusionIntensity(0.2f);
    QCOMPARE(occSpy.size(), 1);
    sound->setOcclusionIntensity(0.2f);
    QCOMPARE(occSpy.size(), 1);

    QSignalSpy dirSpy(sound.get(), &QSpatialSound::directivityChanged);
    sound->setDirectivity(0.3f);
    QCOMPARE(dirSpy.size(), 1);
    sound->setDirectivity(0.3f);
    QCOMPARE(dirSpy.size(), 1);

    QSignalSpy dirOrderSpy(sound.get(), &QSpatialSound::directivityOrderChanged);
    sound->setDirectivityOrder(2.0f);
    QCOMPARE(dirOrderSpy.size(), 1);
    sound->setDirectivityOrder(2.0f);
    QCOMPARE(dirOrderSpy.size(), 1);

    QSignalSpy nfSpy(sound.get(), &QSpatialSound::nearFieldGainChanged);
    sound->setNearFieldGain(0.5f);
    QCOMPARE(nfSpy.size(), 1);
    sound->setNearFieldGain(0.5f);
    QCOMPARE(nfSpy.size(), 1);
}

void tst_QSpatialAudio::test_QSpatialSound_rotation_and_directivityOrder_semantics()
{
    // rotation semantics: should emit only on change
    QSignalSpy rotationSpy(sound.get(), &QSpatialSound::rotationChanged);
    QQuaternion q(1, 1, 0, 0);
    sound->setRotation(q);
    QVERIFY(rotationSpy.size() >= 1);
    const int rotBefore = rotationSpy.size();
    sound->setRotation(q);
    QCOMPARE(rotationSpy.size(), rotBefore);

    QSignalSpy ordSpy(sound.get(), &QSpatialSound::directivityOrderChanged);

    const float orderVal = 3.0f;
    const int ordBefore = ordSpy.size();
    sound->setDirectivityOrder(orderVal);
    const int ordAfter = ordSpy.size();
    QCOMPARE(ordAfter - ordBefore, 1);

    sound->setDirectivityOrder(orderVal);
    QCOMPARE(ordAfter, ordSpy.size());
}

void tst_QSpatialAudio::testEngineIntegration()
{
    QList outputs = QMediaDevices::audioOutputs();
    if (outputs.isEmpty())
        QSKIP("No audio outputs available");

    // basic engine lifecycle
    engine->start();
    QAudioEngine eng;
    eng.start();
    eng.stop();
}

void tst_QSpatialAudio::testSetSource_autoPlay()
{
    QList outputs = QMediaDevices::audioOutputs();
    if (outputs.isEmpty())
        QSKIP("No audio outputs available");

    sound->setAutoPlay(false);
    QSignalSpy srcSpy(sound.get(), &QSpatialSound::sourceChanged);
    sound->setSource(url);
    QCOMPARE(srcSpy.size(), 1);
    // autoPlay is false, so playback state shouldn't start automatically
    // We only check that calling play doesn't crash and can be invoked
    sound->play();
    QTest::qWait(1000); // allow 1 second of playback
    sound->stop();
}

void tst_QSpatialAudio::testQAmbientSound_basic()
{
    QList outputs = QMediaDevices::audioOutputs();
    if (outputs.isEmpty())
        QSKIP("No audio outputs available");

    auto ambient = std::make_unique<QAmbientSound>(engine.get());

    QSignalSpy srcSpy(ambient.get(), &QAmbientSound::sourceChanged);
    QSignalSpy volSpy(ambient.get(), &QAmbientSound::volumeChanged);

    ambient->setAutoPlay(false);
    QCOMPARE(ambient->autoPlay(), false);

    ambient->setVolume(0.4f);
    QCOMPARE(ambient->volume(), 0.4f);
    QCOMPARE(volSpy.size(), 1);

    if (!url.isEmpty()) {
        ambient->setSource(url);
        QCOMPARE(srcSpy.size(), 1);
    }

    ambient->play();
    QTest::qWait(1000);
    ambient->stop();
}

void tst_QSpatialAudio::testQAudioRoom_basic()
{
    // exercise QAudioRoom properties and signals
    auto room = std::make_unique<QAudioRoom>(engine.get());

    // Position
    QSignalSpy posSpy(room.get(), &QAudioRoom::positionChanged);
    QVector3D newPos(1.0f, 2.0f, 3.0f);
    room->setPosition(newPos);
    QCOMPARE(room->position(), newPos);
    QCOMPARE(posSpy.size(), 1);
    room->setPosition(newPos);
    QCOMPARE(posSpy.size(), 1);

    // Dimensions
    QSignalSpy dimSpy(room.get(), &QAudioRoom::dimensionsChanged);
    QVector3D dims(10.0f, 11.0f, 12.0f);
    room->setDimensions(dims);
    QCOMPARE(room->dimensions(), dims);
    QCOMPARE(dimSpy.size(), 1);
    room->setDimensions(dims);
    QCOMPARE(dimSpy.size(), 1);

    // Rotation
    QSignalSpy rotSpy(room.get(), &QAudioRoom::rotationChanged);
    QQuaternion rot(1.0f, 1.0f, 0.0f, 0.0f);
    room->setRotation(rot);
    QCOMPARE(room->rotation(), rot);
    QCOMPARE(rotSpy.size(), 1);
    room->setRotation(rot);
    QCOMPARE(rotSpy.size(), 1);

    // Wall material + wallsChanged
    QSignalSpy wallsSpy(room.get(), &QAudioRoom::wallsChanged);
    room->setWallMaterial(QAudioRoom::LeftWall, QAudioRoom::BrickPainted);
    QCOMPARE(room->wallMaterial(QAudioRoom::LeftWall), QAudioRoom::BrickPainted);
    QCOMPARE(wallsSpy.size(), 1);
    room->setWallMaterial(QAudioRoom::LeftWall, QAudioRoom::BrickPainted);
    QCOMPARE(wallsSpy.size(), 1);

    // Reflection / Reverb properties
    QSignalSpy reflSpy(room.get(), &QAudioRoom::reflectionGainChanged);
    QSignalSpy revGainSpy(room.get(), &QAudioRoom::reverbGainChanged);
    QSignalSpy revTimeSpy(room.get(), &QAudioRoom::reverbTimeChanged);
    QSignalSpy revBrightSpy(room.get(), &QAudioRoom::reverbBrightnessChanged);

    room->setReflectionGain(0.25f);
    QCOMPARE(room->reflectionGain(), 0.25f);
    QCOMPARE(reflSpy.size(), 1);
    room->setReflectionGain(0.25f);
    QCOMPARE(reflSpy.size(), 1);

    room->setReverbGain(0.4f);
    QCOMPARE(room->reverbGain(), 0.4f);
    QCOMPARE(revGainSpy.size(), 1);
    room->setReverbGain(0.4f);
    QCOMPARE(revGainSpy.size(), 1);

    room->setReverbTime(1.2f);
    QCOMPARE(room->reverbTime(), 1.2f);
    QCOMPARE(revTimeSpy.size(), 1);
    room->setReverbTime(1.2f);
    QCOMPARE(revTimeSpy.size(), 1);

    room->setReverbBrightness(0.75f);
    QCOMPARE(room->reverbBrightness(), 0.75f);
    QCOMPARE(revBrightSpy.size(), 1);
    room->setReverbBrightness(0.75f);
    QCOMPARE(revBrightSpy.size(), 1);
}

void tst_QSpatialAudio::testQAudioListener_basic()
{
    QAudioEngine eng;
    QAudioListener listener(&eng);
    listener.setPosition({0,0,0});
    QCOMPARE(listener.position(), QVector3D(0,0,0));
}

void tst_QSpatialAudio::testSwitchAudioDevice()
{
    // Skip if less than 2 audio devices are available
    QList outputs = QMediaDevices::audioOutputs();
    if (outputs.size() < 2)
        QSKIP("Needs at least 2 audio outputs");

    // Get a non-default device
    QAudioDevice nonDefaultDevice = [&] {
        if (!outputs[0].isDefault())
            return outputs[0];
        return outputs[1];
    }();
    QAudioDevice anotherDevice = [&] {
        return outputs[(outputs.indexOf(nonDefaultDevice) + 1) % outputs.size()];
    }();

    // Test 1: Switch device when engine is not running (should succeed)
    engine->setOutputDevice(nonDefaultDevice);
    QCOMPARE(engine->outputDevice(), nonDefaultDevice);

    // Test 2: Switch device when engine is running (should warn)
    QAudioDevice originalDevice = engine->outputDevice();

    engine->start();

    QTest::ignoreMessage(QtMsgType::QtWarningMsg,
                         "Changing device on a running engine not implemented");

    engine->setOutputDevice(anotherDevice);
    QCOMPARE(engine->outputDevice(), originalDevice);

    engine->stop();
}

QTEST_MAIN(tst_QSpatialAudio)

#include "tst_qspatialaudio.moc"
