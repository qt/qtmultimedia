/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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
#include <QStringList>

#include <private/qmediaserviceprovider_p.h>
#include <qmediaserviceproviderplugin.h>
#include <qmediaobject.h>
#include <qmediaservice.h>
#include <qmediaplayer.h>
#include <qaudiorecorder.h>
#include <qcamera.h>
#include <qcamerainfo.h>

QT_USE_NAMESPACE

class MockMediaServiceProvider : public QMediaServiceProvider
{
    QMediaService* requestService(const QByteArray &type) override
    {
        Q_UNUSED(type);
        return nullptr;
    }

    void releaseService(QMediaService *service) override
    {
        Q_UNUSED(service);
    }
};


class tst_QMediaServiceProvider : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();

private slots:
    void testDefaultProviderAvailable();
    void testObtainService();
    void testHasSupport();
    void testSupportedMimeTypes();
    void testDefaultDevice();
    void testAvailableDevices();
    void testCameraInfo();

private:
    QObjectList plugins;
};

void tst_QMediaServiceProvider::initTestCase()
{
//    QMediaPluginLoader::setStaticPlugins(QLatin1String("mediaservice"), plugins);
#if QT_CONFIG(library)
    QCoreApplication::setLibraryPaths(QStringList() << QCoreApplication::applicationDirPath());
#endif
}

void tst_QMediaServiceProvider::testDefaultProviderAvailable()
{
    // Must always be a default provider available
    QVERIFY(QMediaServiceProvider::defaultServiceProvider() != nullptr);
}

void tst_QMediaServiceProvider::testObtainService()
{
    QMediaServiceProvider *provider = QMediaServiceProvider::defaultServiceProvider();

    if (provider == nullptr)
        QSKIP("No default provider");

    QMediaService *service = nullptr;

    // Player
    service = provider->requestService(Q_MEDIASERVICE_MEDIAPLAYER);
    QVERIFY(service != nullptr);
    provider->releaseService(service);
}

void tst_QMediaServiceProvider::testHasSupport()
{
    MockMediaServiceProvider mockProvider;
    QCOMPARE(mockProvider.hasSupport(QByteArray(Q_MEDIASERVICE_MEDIAPLAYER), "video/ogv", QStringList()),
             QMultimedia::MaybeSupported);

    QMediaServiceProvider *provider = QMediaServiceProvider::defaultServiceProvider();

    if (provider == nullptr)
        QSKIP("No default provider");

    QCOMPARE(provider->hasSupport(QByteArray(Q_MEDIASERVICE_MEDIAPLAYER), "video/ogv", QStringList()),
             QMultimedia::MaybeSupported);

    QCOMPARE(provider->hasSupport(QByteArray(Q_MEDIASERVICE_MEDIAPLAYER), "audio/ogg", QStringList()),
             QMultimedia::ProbablySupported);

    //while the service returns PreferredService, provider should return ProbablySupported
    QCOMPARE(provider->hasSupport(QByteArray(Q_MEDIASERVICE_MEDIAPLAYER), "audio/wav", QStringList()),
             QMultimedia::ProbablySupported);

    //even while all the plugins with "hasSupport" returned NotSupported,
    //MockServicePlugin3 has no "hasSupport" interface, so MaybeSupported
    QCOMPARE(provider->hasSupport(QByteArray(Q_MEDIASERVICE_MEDIAPLAYER), "video/avi",
                                  QStringList() << "mpeg4"),
             QMultimedia::MaybeSupported);

    QCOMPARE(provider->hasSupport(QByteArray("non existing service"), "video/ogv", QStringList()),
             QMultimedia::NotSupported);

    QCOMPARE(QMediaPlayer::hasSupport("video/ogv"), QMultimedia::MaybeSupported);
    QCOMPARE(QMediaPlayer::hasSupport("audio/ogg"), QMultimedia::ProbablySupported);
    QCOMPARE(QMediaPlayer::hasSupport("audio/wav"), QMultimedia::ProbablySupported);

    //test low latency flag support
    QCOMPARE(QMediaPlayer::hasSupport("audio/wav", QStringList(), QMediaPlayer::LowLatency),
             QMultimedia::ProbablySupported);
    //plugin1 probably supports audio/ogg, it checked because it doesn't provide features iface
    QCOMPARE(QMediaPlayer::hasSupport("audio/ogg", QStringList(), QMediaPlayer::LowLatency),
             QMultimedia::ProbablySupported);
    //Plugin4 is not checked here, sine it's known not support low latency
    QCOMPARE(QMediaPlayer::hasSupport("video/quicktime", QStringList(), QMediaPlayer::LowLatency),
             QMultimedia::MaybeSupported);

    //test streaming flag support
    QCOMPARE(QMediaPlayer::hasSupport("video/quicktime", QStringList(), QMediaPlayer::StreamPlayback),
             QMultimedia::ProbablySupported);
    //Plugin2 is not checked here, sine it's known not support streaming
    QCOMPARE(QMediaPlayer::hasSupport("audio/wav", QStringList(), QMediaPlayer::StreamPlayback),
             QMultimedia::MaybeSupported);

    //ensure the correct media player plugin is chosen for mime type
    QMediaPlayer simplePlayer(nullptr, QMediaPlayer::LowLatency);
    QCOMPARE(simplePlayer.service()->objectName(), QLatin1String("MockServicePlugin2"));

    QMediaPlayer mediaPlayer;
    QVERIFY(mediaPlayer.service()->objectName() != QLatin1String("MockServicePlugin2"));

    QMediaPlayer streamPlayer(nullptr, QMediaPlayer::StreamPlayback);
    QCOMPARE(streamPlayer.service()->objectName(), QLatin1String("MockServicePlugin4"));
}

void tst_QMediaServiceProvider::testSupportedMimeTypes()
{
    QMediaServiceProvider *provider = QMediaServiceProvider::defaultServiceProvider();

    if (provider == nullptr)
        QSKIP("No default provider");

    QVERIFY(provider->supportedMimeTypes(QByteArray(Q_MEDIASERVICE_MEDIAPLAYER)).contains("audio/ogg"));
    QVERIFY(!provider->supportedMimeTypes(QByteArray(Q_MEDIASERVICE_MEDIAPLAYER)).contains("audio/mp3"));
}

void tst_QMediaServiceProvider::testDefaultDevice()
{
    QMediaServiceProvider *provider = QMediaServiceProvider::defaultServiceProvider();

    if (provider == nullptr)
        QSKIP("No default provider");

    QCOMPARE(provider->defaultDevice(Q_MEDIASERVICE_AUDIOSOURCE), QByteArray("audiosource1"));
    QCOMPARE(provider->defaultDevice(Q_MEDIASERVICE_CAMERA), QByteArray("frontcamera"));
}

void tst_QMediaServiceProvider::testAvailableDevices()
{
    QMediaServiceProvider *provider = QMediaServiceProvider::defaultServiceProvider();

    if (provider == nullptr)
        QSKIP("No default provider");

    QList<QByteArray> devices = provider->devices(Q_MEDIASERVICE_AUDIOSOURCE);
    QCOMPARE(devices.count(), 2);
    QCOMPARE(devices.at(0), QByteArray("audiosource1"));
    QCOMPARE(devices.at(1), QByteArray("audiosource2"));

    devices = provider->devices(Q_MEDIASERVICE_CAMERA);
    QCOMPARE(devices.count(), 3);
    QCOMPARE(devices.at(0), QByteArray("frontcamera"));
    QCOMPARE(devices.at(1), QByteArray("backcamera"));
    QCOMPARE(devices.at(2), QByteArray("somecamera"));
}

void tst_QMediaServiceProvider::testCameraInfo()
{
    QMediaServiceProvider *provider = QMediaServiceProvider::defaultServiceProvider();

    if (provider == nullptr)
        QSKIP("No default provider");

    QCOMPARE(provider->cameraPosition("backcamera"), QCamera::BackFace);
    QCOMPARE(provider->cameraOrientation("backcamera"), 90);
    QCOMPARE(provider->cameraPosition("frontcamera"), QCamera::FrontFace);
    QCOMPARE(provider->cameraOrientation("frontcamera"), 270);
    QCOMPARE(provider->cameraPosition("somecamera"), QCamera::UnspecifiedPosition);
    QCOMPARE(provider->cameraOrientation("somecamera"), 0);

    {
        QCamera camera;
        QVERIFY(camera.service());
        QCOMPARE(camera.service()->objectName(), QLatin1String("MockServicePlugin3"));
    }

    {
        QCamera camera(QCameraInfo::defaultCamera());
        QVERIFY(camera.service());
        QCOMPARE(camera.service()->objectName(), QLatin1String("MockServicePlugin3"));
    }

    {
        QCamera camera(QCameraInfo::availableCameras().at(0));
        QVERIFY(camera.service());
        QCOMPARE(camera.service()->objectName(), QLatin1String("MockServicePlugin3"));
    }

    {
        QCamera camera(QCameraInfo::availableCameras().at(1));
        QVERIFY(camera.service());
        QCOMPARE(camera.service()->objectName(), QLatin1String("MockServicePlugin5"));
    }

    {
        QCamera camera(QCameraInfo::availableCameras().at(2));
        QVERIFY(camera.service());
        QCOMPARE(camera.service()->objectName(), QLatin1String("MockServicePlugin5"));
    }

    {
        QCamera camera(QCamera::FrontFace);
        QVERIFY(camera.service());
        QCOMPARE(camera.service()->objectName(), QLatin1String("MockServicePlugin3"));
    }

    {
        QCamera camera(QCamera::BackFace);
        QVERIFY(camera.service());
        QCOMPARE(camera.service()->objectName(), QLatin1String("MockServicePlugin5"));
    }

    {
        QCamera camera(QCamera::UnspecifiedPosition);
        QVERIFY(camera.service());
        QCOMPARE(camera.service()->objectName(), QLatin1String("MockServicePlugin3"));
    }
}

QTEST_MAIN(tst_QMediaServiceProvider)

#include "tst_qmediaserviceprovider.moc"
