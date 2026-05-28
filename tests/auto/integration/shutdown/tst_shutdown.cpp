// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtMultimedia/qaudiodecoder.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudioinput.h>
#include <QtMultimedia/qcamera.h>
#include <QtMultimedia/qmediacapturesession.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/qmediaplayer.h>
#include <QtMultimedia/qmediarecorder.h>
#include <QtMultimedia/qsoundeffect.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>
#include <QtGui/qguiapplication.h>

using namespace Qt::StringLiterals;

QT_USE_NAMESPACE

namespace {

template <typename Functor>
auto withQGuiApplication(Functor &&f)
{
    static int argc = 1;
    static char **argv = nullptr;
    auto app = QGuiApplication{
        argc,
        argv,
    };
    return f();
}

} // namespace

class tst_shutdown : public QObject
{
    Q_OBJECT

private slots:
    void soundEffect_doesNotCrash_whenOwnedByQApp();
    void mediaPlayer_doesNotCrash_whenOwnedByQApp();
    void audioDecoder_doesNotCrash_whenOwnedByQApp();
    void captureSession_doesNotCrash_whenOwnedByQApp();
};

void tst_shutdown::soundEffect_doesNotCrash_whenOwnedByQApp()
{
    withQGuiApplication([] {
        auto effect = new QSoundEffect(qApp);
        effect->setSource(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")));
        effect->setLoopCount(QSoundEffect::Infinite);
        effect->play();
        QTest::qWait(100);
    });
}

void tst_shutdown::mediaPlayer_doesNotCrash_whenOwnedByQApp()
{
    withQGuiApplication([] {
        auto player = new QMediaPlayer(qApp);
        player->setSource(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.mp4")));
        player->setLoops(-1);
        player->play();
        QTest::qWait(100);
    });
}

void tst_shutdown::audioDecoder_doesNotCrash_whenOwnedByQApp()
{
    withQGuiApplication([] {
        auto decoder = new QAudioDecoder(qApp);
        decoder->setSource(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")));
        decoder->start();
        QTest::qWait(100);
    });
}

void tst_shutdown::captureSession_doesNotCrash_whenOwnedByQApp()
{
    withQGuiApplication([] {
        auto session = new QMediaCaptureSession(qApp);

        auto audioInputs = QMediaDevices::audioInputs();
        if (!audioInputs.isEmpty()) {
            auto audioInput = new QAudioInput(audioInputs.first(), qApp);
            session->setAudioInput(audioInput);
        }

        auto recorder = new QMediaRecorder(qApp);
        session->setRecorder(recorder);

        if (!audioInputs.isEmpty())
            recorder->record();

        QTest::qWait(100);
    });
}

QTEST_APPLESS_MAIN(tst_shutdown)

#include "tst_shutdown.moc"
