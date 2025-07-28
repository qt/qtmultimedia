// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>

// Adds an enum, and the stringized version
#define ADD_ENUM_TEST(x) \
    QTest::newRow(#x) \
        << QAudio::x \
    << QString(QLatin1String(#x));

class tst_QAudioNamespace : public QObject
{
    Q_OBJECT

private slots:
    void debugError();
    void debugError_data();
    void debugState();
    void debugState_data();
    void debugVolumeScale();
    void debugVolumeScale_data();
    void convertVolume();
    void convertVolume_data();
};

void tst_QAudioNamespace::debugError_data()
{
    QTest::addColumn<QAudio::Error>("error");
    QTest::addColumn<QString>("stringized");

    ADD_ENUM_TEST(NoError);
    ADD_ENUM_TEST(OpenError);
    ADD_ENUM_TEST(IOError);
    ADD_ENUM_TEST(UnderrunError);
    ADD_ENUM_TEST(FatalError);
}

void tst_QAudioNamespace::debugError()
{
    QFETCH(QAudio::Error, error);
    QFETCH(QString, stringized);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << error;
}

void tst_QAudioNamespace::debugState_data()
{
    QTest::addColumn<QAudio::State>("state");
    QTest::addColumn<QString>("stringized");

    ADD_ENUM_TEST(ActiveState);
    ADD_ENUM_TEST(SuspendedState);
    ADD_ENUM_TEST(StoppedState);
    ADD_ENUM_TEST(IdleState);
}

void tst_QAudioNamespace::debugState()
{
    QFETCH(QAudio::State, state);
    QFETCH(QString, stringized);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << state;
}

void tst_QAudioNamespace::debugVolumeScale_data()
{
    QTest::addColumn<QAudio::VolumeScale>("volumeScale");
    QTest::addColumn<QString>("stringized");

    ADD_ENUM_TEST(LinearVolumeScale);
    ADD_ENUM_TEST(CubicVolumeScale);
    ADD_ENUM_TEST(DecibelVolumeScale);
}

void tst_QAudioNamespace::debugVolumeScale()
{
    QFETCH(QAudio::VolumeScale, volumeScale);
    QFETCH(QString, stringized);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << volumeScale;
}

void tst_QAudioNamespace::convertVolume_data()
{
    QTest::addColumn<qreal>("input");
    QTest::addColumn<QAudio::VolumeScale>("from");
    QTest::addColumn<QAudio::VolumeScale>("to");
    QTest::addColumn<qreal>("expectedOutput");

    QTest::newRow("-1.0 from linear to linear") << qreal(-1.0) << QAudio::LinearVolumeScale << QAudio::LinearVolumeScale << qreal(0.0);
    QTest::newRow("0.0 from linear to linear") << qreal(0.0) << QAudio::LinearVolumeScale << QAudio::LinearVolumeScale << qreal(0.0);
    QTest::newRow("0.5 from linear to linear") << qreal(0.5) << QAudio::LinearVolumeScale << QAudio::LinearVolumeScale << qreal(0.5);
    QTest::newRow("1.0 from linear to linear") << qreal(1.0) << QAudio::LinearVolumeScale << QAudio::LinearVolumeScale << qreal(1.0);

    QTest::newRow("-1.0 from linear to cubic") << qreal(-1.0) << QAudio::LinearVolumeScale << QAudio::CubicVolumeScale << qreal(0.0);
    QTest::newRow("0.0 from linear to cubic") << qreal(0.0) << QAudio::LinearVolumeScale << QAudio::CubicVolumeScale << qreal(0.0);
    QTest::newRow("0.33 from linear to cubic") << qreal(0.33) << QAudio::LinearVolumeScale << QAudio::CubicVolumeScale << qreal(0.69);
    QTest::newRow("0.5 from linear to cubic") << qreal(0.5) << QAudio::LinearVolumeScale << QAudio::CubicVolumeScale << qreal(0.79);
    QTest::newRow("0.72 from linear to cubic") << qreal(0.72) << QAudio::LinearVolumeScale << QAudio::CubicVolumeScale << qreal(0.89);
    QTest::newRow("1.0 from linear to cubic") << qreal(1.0) << QAudio::LinearVolumeScale << QAudio::CubicVolumeScale << qreal(1.0);

    QTest::newRow("-1.0 from linear to logarithmic") << qreal(-1.0) << QAudio::LinearVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.0);
    QTest::newRow("0.0 from linear to logarithmic") << qreal(0.0) << QAudio::LinearVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.0);
    QTest::newRow("0.33 from linear to logarithmic") << qreal(0.33) << QAudio::LinearVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.78);
    QTest::newRow("0.5 from linear to logarithmic") << qreal(0.5) << QAudio::LinearVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.9);
    QTest::newRow("0.72 from linear to logarithmic") << qreal(0.72) << QAudio::LinearVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.96);
    QTest::newRow("1.0 from linear to logarithmic") << qreal(1.0) << QAudio::LinearVolumeScale << QAudio::LogarithmicVolumeScale << qreal(1.0);

    QTest::newRow("-1.0 from linear to decibel") << qreal(-1.0) << QAudio::LinearVolumeScale << QAudio::DecibelVolumeScale << qreal(-200);
    QTest::newRow("0.0 from linear to decibel") << qreal(0.0) << QAudio::LinearVolumeScale << QAudio::DecibelVolumeScale << qreal(-200);
    QTest::newRow("0.33 from linear to decibel") << qreal(0.33) << QAudio::LinearVolumeScale << QAudio::DecibelVolumeScale << qreal(-9.63);
    QTest::newRow("0.5 from linear to decibel") << qreal(0.5) << QAudio::LinearVolumeScale << QAudio::DecibelVolumeScale << qreal(-6.02);
    QTest::newRow("0.72 from linear to decibel") << qreal(0.72) << QAudio::LinearVolumeScale << QAudio::DecibelVolumeScale << qreal(-2.85);
    QTest::newRow("1.0 from linear to decibel") << qreal(1.0) << QAudio::LinearVolumeScale << QAudio::DecibelVolumeScale << qreal(0);

    QTest::newRow("-1.0 from cubic to linear") << qreal(-1.0) << QAudio::CubicVolumeScale << QAudio::LinearVolumeScale << qreal(0.0);
    QTest::newRow("0.0 from cubic to linear") << qreal(0.0) << QAudio::CubicVolumeScale << QAudio::LinearVolumeScale << qreal(0.0);
    QTest::newRow("0.33 from cubic to linear") << qreal(0.33) << QAudio::CubicVolumeScale << QAudio::LinearVolumeScale << qreal(0.04);
    QTest::newRow("0.5 from cubic to linear") << qreal(0.5) << QAudio::CubicVolumeScale << QAudio::LinearVolumeScale << qreal(0.13);
    QTest::newRow("0.72 from cubic to linear") << qreal(0.72) << QAudio::CubicVolumeScale << QAudio::LinearVolumeScale << qreal(0.37);
    QTest::newRow("1.0 from cubic to linear") << qreal(1.0) << QAudio::CubicVolumeScale << QAudio::LinearVolumeScale << qreal(1.0);

    QTest::newRow("-1.0 from cubic to cubic") << qreal(-1.0) << QAudio::CubicVolumeScale << QAudio::CubicVolumeScale << qreal(0.0);
    QTest::newRow("0.0 from cubic to cubic") << qreal(0.0) << QAudio::CubicVolumeScale << QAudio::CubicVolumeScale << qreal(0.0);
    QTest::newRow("0.5 from cubic to cubic") << qreal(0.5) << QAudio::CubicVolumeScale << QAudio::CubicVolumeScale << qreal(0.5);
    QTest::newRow("1.0 from cubic to cubic") << qreal(1.0) << QAudio::CubicVolumeScale << QAudio::CubicVolumeScale << qreal(1.0);

    QTest::newRow("-1.0 from cubic to logarithmic") << qreal(-1.0) << QAudio::CubicVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.0);
    QTest::newRow("0.0 from cubic to logarithmic") << qreal(0.0) << QAudio::CubicVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.0);
    QTest::newRow("0.33 from cubic to logarithmic") << qreal(0.33) << QAudio::CubicVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.15);
    QTest::newRow("0.5 from cubic to logarithmic") << qreal(0.5) << QAudio::CubicVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.44);
    QTest::newRow("0.72 from cubic to logarithmic") << qreal(0.72) << QAudio::CubicVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.82);
    QTest::newRow("1.0 from cubic to logarithmic") << qreal(1.0) << QAudio::CubicVolumeScale << QAudio::LogarithmicVolumeScale << qreal(1.0);

    QTest::newRow("-1.0 from cubic to decibel") << qreal(-1.0) << QAudio::CubicVolumeScale << QAudio::DecibelVolumeScale << qreal(-200);
    QTest::newRow("0.0 from cubic to decibel") << qreal(0.0) << QAudio::CubicVolumeScale << QAudio::DecibelVolumeScale << qreal(-200);
    QTest::newRow("0.33 from cubic to decibel") << qreal(0.33) << QAudio::CubicVolumeScale << QAudio::DecibelVolumeScale << qreal(-28.89);
    QTest::newRow("0.5 from cubic to decibel") << qreal(0.5) << QAudio::CubicVolumeScale << QAudio::DecibelVolumeScale << qreal(-18.06);
    QTest::newRow("0.72 from cubic to decibel") << qreal(0.72) << QAudio::CubicVolumeScale << QAudio::DecibelVolumeScale << qreal(-8.56);
    QTest::newRow("1.0 from cubic to decibel") << qreal(1.0) << QAudio::CubicVolumeScale << QAudio::DecibelVolumeScale << qreal(0);

    QTest::newRow("-1.0 from logarithmic to linear") << qreal(-1.0) << QAudio::LogarithmicVolumeScale << QAudio::LinearVolumeScale << qreal(0.0);
    QTest::newRow("0.0 from logarithmic to linear") << qreal(0.0) << QAudio::LogarithmicVolumeScale << QAudio::LinearVolumeScale << qreal(0.0);
    QTest::newRow("0.33 from logarithmic to linear") << qreal(0.33) << QAudio::LogarithmicVolumeScale << QAudio::LinearVolumeScale << qreal(0.09);
    QTest::newRow("0.5 from logarithmic to linear") << qreal(0.5) << QAudio::LogarithmicVolumeScale << QAudio::LinearVolumeScale << qreal(0.15);
    QTest::newRow("0.72 from logarithmic to linear") << qreal(0.72) << QAudio::LogarithmicVolumeScale << QAudio::LinearVolumeScale << qreal(0.28);
    QTest::newRow("1.0 from logarithmic to linear") << qreal(1.0) << QAudio::LogarithmicVolumeScale << QAudio::LinearVolumeScale << qreal(1.0);

    QTest::newRow("-1.0 from logarithmic to cubic") << qreal(-1.0) << QAudio::LogarithmicVolumeScale << QAudio::CubicVolumeScale << qreal(0.0);
    QTest::newRow("0.0 from logarithmic to cubic") << qreal(0.0) << QAudio::LogarithmicVolumeScale << QAudio::CubicVolumeScale << qreal(0.0);
    QTest::newRow("0.33 from logarithmic to cubic") << qreal(0.33) << QAudio::LogarithmicVolumeScale << QAudio::CubicVolumeScale << qreal(0.44);
    QTest::newRow("0.5 from logarithmic to cubic") << qreal(0.5) << QAudio::LogarithmicVolumeScale << QAudio::CubicVolumeScale << qreal(0.53);
    QTest::newRow("0.72 from logarithmic to cubic") << qreal(0.72) << QAudio::LogarithmicVolumeScale << QAudio::CubicVolumeScale << qreal(0.65);
    QTest::newRow("1.0 from logarithmic to cubic") << qreal(1.0) << QAudio::LogarithmicVolumeScale << QAudio::CubicVolumeScale << qreal(1.0);

    QTest::newRow("-1.0 from logarithmic to logarithmic") << qreal(-1.0) << QAudio::LogarithmicVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.0);
    QTest::newRow("0.0 from logarithmic to logarithmic") << qreal(0.0) << QAudio::LogarithmicVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.0);
    QTest::newRow("0.5 from logarithmic to logarithmic") << qreal(0.5) << QAudio::LogarithmicVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.5);
    QTest::newRow("1.0 from logarithmic to logarithmic") << qreal(1.0) << QAudio::LogarithmicVolumeScale << QAudio::LogarithmicVolumeScale << qreal(1.0);

    QTest::newRow("-1.0 from logarithmic to decibel") << qreal(-1.0) << QAudio::LogarithmicVolumeScale << QAudio::DecibelVolumeScale << qreal(-200);
    QTest::newRow("0.0 from logarithmic to decibel") << qreal(0.0) << QAudio::LogarithmicVolumeScale << QAudio::DecibelVolumeScale << qreal(-200);
    QTest::newRow("0.33 from logarithmic to decibel") << qreal(0.33) << QAudio::LogarithmicVolumeScale << QAudio::DecibelVolumeScale << qreal(-21.21);
    QTest::newRow("0.5 from logarithmic to decibel") << qreal(0.5) << QAudio::LogarithmicVolumeScale << QAudio::DecibelVolumeScale << qreal(-16.45);
    QTest::newRow("0.72 from logarithmic to decibel") << qreal(0.72) << QAudio::LogarithmicVolumeScale << QAudio::DecibelVolumeScale << qreal(-11.17);
    QTest::newRow("1.0 from logarithmic to decibel") << qreal(1.0) << QAudio::LogarithmicVolumeScale << QAudio::DecibelVolumeScale << qreal(0);

    QTest::newRow("-1000 from decibel to linear") << qreal(-1000.0) << QAudio::DecibelVolumeScale << QAudio::LinearVolumeScale << qreal(0.0);
    QTest::newRow("-200 from decibel to linear") << qreal(-200.0) << QAudio::DecibelVolumeScale << QAudio::LinearVolumeScale << qreal(0.0);
    QTest::newRow("-40 from decibel to linear") << qreal(-40.0) << QAudio::DecibelVolumeScale << QAudio::LinearVolumeScale << qreal(0.01);
    QTest::newRow("-10 from decibel to linear") << qreal(-10.0) << QAudio::DecibelVolumeScale << QAudio::LinearVolumeScale << qreal(0.32);
    QTest::newRow("-5 from decibel to linear") << qreal(-5.0) << QAudio::DecibelVolumeScale << QAudio::LinearVolumeScale << qreal(0.56);
    QTest::newRow("0 from decibel to linear") << qreal(0.0) << QAudio::DecibelVolumeScale << QAudio::LinearVolumeScale << qreal(1.0);

    QTest::newRow("-1000 from decibel to cubic") << qreal(-1000.0) << QAudio::DecibelVolumeScale << QAudio::CubicVolumeScale << qreal(0.0);
    QTest::newRow("-200 from decibel to cubic") << qreal(-200.0) << QAudio::DecibelVolumeScale << QAudio::CubicVolumeScale << qreal(0.0);
    QTest::newRow("-40 from decibel to cubic") << qreal(-40.0) << QAudio::DecibelVolumeScale << QAudio::CubicVolumeScale << qreal(0.22);
    QTest::newRow("-10 from decibel to cubic") << qreal(-10.0) << QAudio::DecibelVolumeScale << QAudio::CubicVolumeScale << qreal(0.68);
    QTest::newRow("-5 from decibel to cubic") << qreal(-5.0) << QAudio::DecibelVolumeScale << QAudio::CubicVolumeScale << qreal(0.83);
    QTest::newRow("0 from decibel to cubic") << qreal(0.0) << QAudio::DecibelVolumeScale << QAudio::CubicVolumeScale << qreal(1.0);

    QTest::newRow("-1000 from decibel to logarithmic") << qreal(-1000.0) << QAudio::DecibelVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.0);
    QTest::newRow("-200 from decibel to logarithmic") << qreal(-200.0) << QAudio::DecibelVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.0);
    QTest::newRow("-40 from decibel to logarithmic") << qreal(-40.0) << QAudio::DecibelVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.05);
    QTest::newRow("-10 from decibel to logarithmic") << qreal(-10.0) << QAudio::DecibelVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.77);
    QTest::newRow("-5 from decibel to logarithmic") << qreal(-5.0) << QAudio::DecibelVolumeScale << QAudio::LogarithmicVolumeScale << qreal(0.92);
    QTest::newRow("0 from decibel to logarithmic") << qreal(0.0) << QAudio::DecibelVolumeScale << QAudio::LogarithmicVolumeScale << qreal(1.0);

    QTest::newRow("-1000 from decibel to decibel") << qreal(-1000.0) << QAudio::DecibelVolumeScale << QAudio::DecibelVolumeScale << qreal(-1000.0);
    QTest::newRow("-200 from decibel to decibel") << qreal(-200.0) << QAudio::DecibelVolumeScale << QAudio::DecibelVolumeScale << qreal(-200.0);
    QTest::newRow("-30 from decibel to decibel") << qreal(-30.0) << QAudio::DecibelVolumeScale << QAudio::DecibelVolumeScale << qreal(-30.0);
    QTest::newRow("0 from decibel to decibel") << qreal(0.0) << QAudio::DecibelVolumeScale << QAudio::DecibelVolumeScale << qreal(0.0);
}

void tst_QAudioNamespace::convertVolume()
{
    QFETCH(qreal, input);
    QFETCH(QAudio::VolumeScale, from);
    QFETCH(QAudio::VolumeScale, to);
    QFETCH(qreal, expectedOutput);

    qreal actualOutput = QAudio::convertVolume(input, from, to);
    QVERIFY2(qAbs(actualOutput - expectedOutput) < 0.01,
             QStringLiteral("actual: %1, expected: %2").arg(actualOutput).arg(expectedOutput).toLocal8Bit().constData());
}

QTEST_MAIN(tst_QAudioNamespace)

#include "tst_qaudionamespace.moc"
