// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/qsoundeffect.h>
#include <QtCore/qcommandlineparser.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qprocess.h>
#include <QtCore/qstring.h>

using namespace Qt::StringLiterals;

QT_USE_NAMESPACE

namespace {
bool executeTestOutOfProcess(const QString &testName);
void playSound();
} // namespace

class tst_multiapp : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase()
    {
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        QSKIP("Out-of-process testing does not behave correctly on mobile OS");
#endif
    }

private slots:
    void mediaDevices_doesNotCrash_whenRecreatingApplication()
    {
        QVERIFY(executeTestOutOfProcess(
                "mediaDevices_doesNotCrash_whenRecreatingApplication_impl"_L1));
    }

    bool mediaDevices_doesNotCrash_whenRecreatingApplication_impl(int argc, char ** argv)
    {
        {
            QCoreApplication app{ argc, argv };
            QMediaDevices::defaultAudioOutput();
        }
        {
            QCoreApplication app{ argc, argv };
            QMediaDevices::defaultAudioOutput();
        }

        return true;
    }

    void soundEffect_doesNotCrash_whenRecreatingApplication()
    {
        QVERIFY(executeTestOutOfProcess(
                "soundEffect_doesNotCrash_whenRecreatingApplication_impl"_L1));
    }

    bool soundEffect_doesNotCrash_whenRecreatingApplication_impl(int argc, char **argv)
    {
        QTEST_ASSERT(!qApp);

        // Play a sound twice under two different application objects
        // This verifies that QSoundEffect works in use cases where
        // client application recreates Qt application instances,
        // for example when the client application loads plugins
        // implemented using Qt.
        {
            QCoreApplication app{ argc, argv };
            playSound();
        }
        {
            QCoreApplication app{ argc, argv };
            playSound();
        }

        return true;
    }

};

namespace {

void playSound()
{
    const QUrl url{ "qrc:/double-drop.wav"_L1 };

    QSoundEffect effect;
    effect.setSource(url);
    effect.play();

    QObject::connect(&effect, &QSoundEffect::playingChanged, qApp, [&]() {
        if (!effect.isPlaying())
            qApp->quit();
    });

    // In some CI configurations, we do not have any audio devices. We must therefore
    // close the qApp on error signal instead of on playingChanged.
    QObject::connect(&effect, &QSoundEffect::statusChanged, qApp, [&]() {
        if (effect.status() == QSoundEffect::Status::Error) {
            qDebug() << "Failed to play sound effect";
            qApp->quit();
        }
    });

    qApp->exec();
}

bool executeTestOutOfProcess(const QString &testName)
{
    const QStringList args{ "--run-test"_L1, testName };
    const QString processName = QCoreApplication::applicationFilePath();
    const int status = QProcess::execute(processName, args);
    return status == 0;
}

} // namespace

// This main function executes tests like normal qTest, and adds support
// for executing specific test functions when called out of process. In this
// case we don't create a QApplication, because the intent is to test how features
// behave when no QApplication exists.
int main(int argc, char *argv[])
{
    QCommandLineParser cmd;
    const QCommandLineOption runTest{ QStringList{ "run-test" }, "Executes a named test",
                                      "runTest" };
    cmd.addOption(runTest);
    cmd.parse({ argv, argv + argc });

    if (cmd.isSet(runTest)) {
        // We are requested to run a test case in a separate process without a Qt application
        const QString testName = cmd.value(runTest);

        bool returnValue = false;
        tst_multiapp tc;

        // Call the requested function on the test class
        const bool invokeResult =
                QMetaObject::invokeMethod(&tc, testName.toLatin1(), Qt::DirectConnection,
                                          qReturnArg(returnValue), argc, argv);

        return (invokeResult && returnValue) ? 0 : 1;
    }

    // If no special arguments are set, enter the regular QTest main routine
    // The below lines are the same that QTEST_GUILESS_MAIN would stamp out,
    // except the `int main(...)`
    TESTLIB_SELFCOVERAGE_START("tst_multiapp")
    QT_PREPEND_NAMESPACE(QTest::Internal::callInitMain)<tst_multiapp>();
    QCoreApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    tst_multiapp tc;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_multiapp.moc"
