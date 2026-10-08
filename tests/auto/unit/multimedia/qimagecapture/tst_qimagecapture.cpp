// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QDebug>

#include <private/qplatformcamera_p.h>
#include <private/qplatformimagecapture_p.h>
#include <qcamera.h>
#include <qimagecapture.h>
#include <qmediacapturesession.h>

#include "qmockmediacapturesession.h"
#include "qmockintegration.h"

QT_USE_NAMESPACE

Q_ENABLE_MOCK_MULTIMEDIA_PLUGIN

class tst_QImageCapture: public QObject
{
    Q_OBJECT

private slots:
    void constructor();
    void isAvailable();
    void deleteMediaSource();
    void isReadyForCapture();
    void capture();
    void encodingSettings();
    void errors();
    void error();
    void imageCaptured();
    void imageExposed();
    void imageSaved();
    void readyForCaptureChanged();
};

void tst_QImageCapture::constructor()
{
    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QVERIFY(imageCapture.isAvailable() == true);
}

void tst_QImageCapture::isAvailable()
{
    {
        QMediaCaptureSession session;
        QImageCapture imageCapture;
        session.setImageCapture(&imageCapture);

        QVERIFY(!imageCapture.isAvailable());
    }

    {
        QMediaCaptureSession session;
        QCamera camera;
        QImageCapture imageCapture;
        session.setCamera(&camera);
        session.setImageCapture(&imageCapture);

        QVERIFY(imageCapture.isAvailable());
    }
}

void tst_QImageCapture::deleteMediaSource()
{
    QMediaCaptureSession session;
    QCamera *camera = new QCamera;
    QImageCapture *capture = new QImageCapture;
    session.setCamera(camera);
    session.setImageCapture(capture);

    QVERIFY(capture->isAvailable());

    delete camera;

    QVERIFY(session.camera() == nullptr);
    QVERIFY(!capture->isAvailable());

    capture->captureToFile();
    delete capture;
}

void tst_QImageCapture::isReadyForCapture()
{
    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    camera.start();
    QVERIFY(imageCapture.isReadyForCapture() == true);
    imageCapture.captureToFile();
    QTRY_VERIFY(imageCapture.isReadyForCapture());
    camera.stop();
}

void tst_QImageCapture::capture()
{
    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    QVERIFY(imageCapture.captureToFile() == -1);
    camera.start();
    QVERIFY(imageCapture.isReadyForCapture() == true);
    QTest::qWait(300);
    QVERIFY(imageCapture.captureToFile() != -1);
    camera.stop();
}

void tst_QImageCapture::encodingSettings()
{
    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QVERIFY(imageCapture.isAvailable() == true);
    imageCapture.setFileFormat(QImageCapture::JPEG);
    imageCapture.setQuality(QImageCapture::NormalQuality);
    QVERIFY(imageCapture.fileFormat() == QImageCapture::JPEG);
    QVERIFY(imageCapture.quality() == QImageCapture::NormalQuality);
}

void tst_QImageCapture::errors()
{
    QMockCamera::Simple simple;

    {
        QMediaCaptureSession session;
        QCamera camera;
        QImageCapture imageCapture;
        session.setCamera(&camera);
        session.setImageCapture(&imageCapture);

        QVERIFY(imageCapture.isAvailable() == true);
        imageCapture.captureToFile(QStringLiteral("/dev/null"));
        QCOMPARE(imageCapture.error(), QImageCapture::NotReadyError);
        QVERIFY(!imageCapture.errorString().isEmpty());
    }

    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);
    camera.start();

    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.error() == QImageCapture::NoError);
    QVERIFY(imageCapture.errorString().isEmpty());

    imageCapture.captureToFile();
    QVERIFY(imageCapture.error() == QImageCapture::NoError);
    QVERIFY(imageCapture.errorString().isEmpty());
}

void tst_QImageCapture::error()
{
    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy spy(&imageCapture, &QImageCapture::errorOccurred);
    imageCapture.captureToFile();
    QTest::qWait(30);
    QVERIFY(spy.size() == 1);
    QVERIFY(qvariant_cast<int>(spy.at(0).at(0)) == -1);
    QVERIFY(qvariant_cast<QImageCapture::Error>(spy.at(0).at(1)) == QImageCapture::NotReadyError);
    QVERIFY(qvariant_cast<QString>(spy.at(0).at(2)) == "Could not capture in stopped state");
    spy.clear();
}

void tst_QImageCapture::imageCaptured()
{
    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy spy(&imageCapture, &QImageCapture::imageCaptured);
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    camera.start();
    imageCapture.captureToFile();
    QTRY_VERIFY(imageCapture.isReadyForCapture());

    QVERIFY(spy.size() == 1);
    QVERIFY(qvariant_cast<int>(spy.at(0).at(0)) > 0);
    QImage image = qvariant_cast<QImage>(spy.at(0).at(1));
    QVERIFY(image.isNull() == true);
    spy.clear();
    camera.stop();
}

void tst_QImageCapture::imageExposed()
{
    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy spy(&imageCapture, &QImageCapture::imageExposed);
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    camera.start();
    imageCapture.captureToFile();
    QTRY_VERIFY(imageCapture.isReadyForCapture());

    QVERIFY(spy.size() == 1);
    QVERIFY(qvariant_cast<int>(spy.at(0).at(0)) > 0);
    spy.clear();
    camera.stop();
}

void tst_QImageCapture::imageSaved()
{
    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy spy(&imageCapture, &QImageCapture::imageSaved);
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    camera.start();
    imageCapture.captureToFile(QStringLiteral("/usr/share"));
    QTRY_VERIFY(imageCapture.isReadyForCapture());

    QVERIFY(spy.size() == 1);
    QVERIFY(qvariant_cast<int>(spy.at(0).at(0)) > 0);
    QVERIFY(qvariant_cast<QString>(spy.at(0).at(1)) == "/usr/share");
    spy.clear();
    camera.stop();
}

void tst_QImageCapture::readyForCaptureChanged()
{
    QMediaCaptureSession session;
    QCamera camera;
    QImageCapture imageCapture;
    session.setCamera(&camera);
    session.setImageCapture(&imageCapture);

    QSignalSpy spy(&imageCapture, &QImageCapture::readyForCaptureChanged);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    imageCapture.captureToFile();
    QTest::qWait(100);
    QVERIFY(spy.size() == 0);
    QVERIFY2(!imageCapture.errorString().isEmpty(),"Could not capture in stopped state" );
    camera.start();
    QTest::qWait(100);
    imageCapture.captureToFile();
    QTest::qWait(100);
    QVERIFY(spy.size() == 2);
    QVERIFY(spy.at(0).at(0).toBool() == false);
    QVERIFY(spy.at(1).at(0).toBool() == true);
    camera.stop();
    spy.clear();
}

QTEST_MAIN(tst_QImageCapture)

#include "tst_qimagecapture.moc"
