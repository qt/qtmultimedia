// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/qsoundeffect.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>

using namespace Qt::StringLiterals;

QT_USE_NAMESPACE

namespace {

template <typename Functor>
auto withQCoreApplication(Functor &&f)
{
    static int argc = 1;
    static char **argv = nullptr;
    auto app = QCoreApplication{
        argc,
        argv,
    };
    return f();
}

} // namespace

class tst_multiapp : public QObject
{
    Q_OBJECT

private slots:
    void mediaDevices_doesNotCrash_whenRecreatingApplication();
    void soundEffect_doesNotCrash_whenRecreatingApplication();
    void soundEffect_doesNotCrash_whenDestroyingApplication();
};

void tst_multiapp::mediaDevices_doesNotCrash_whenRecreatingApplication()
{
    withQCoreApplication([] {
        QMediaDevices::defaultAudioOutput();
    });

    withQCoreApplication([] {
        QMediaDevices::defaultAudioOutput();
    });
}

void tst_multiapp::soundEffect_doesNotCrash_whenRecreatingApplication()
{
    for (int i = 0; i != 2; ++i) {
        withQCoreApplication([] {
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
        });
    }
}

void tst_multiapp::soundEffect_doesNotCrash_whenDestroyingApplication()
{
    withQCoreApplication([] {
        QSoundEffect sound;
        sound.setSource(QUrl(u"yada://test"_s)); // force loading via QNetworkAccessManager
    });
}

QTEST_APPLESS_MAIN(tst_multiapp)

#include "tst_multiapp.moc"
