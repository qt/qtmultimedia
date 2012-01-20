/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
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

/****************************************************************************
Author : Vijay/Avinash

Reviewer Name       Date                Coverage ( Full / Test Case IDs ).
---------------------------------------------------------------------------
                                        Initial review of test cases.
****************************************************************************/

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <QDebug>

#include <qcameracontrol.h>
#include <qcameralockscontrol.h>
#include <qcameraexposurecontrol.h>
#include <qcameraflashcontrol.h>
#include <qcamerafocuscontrol.h>
#include <qcameraimagecapturecontrol.h>
#include <qimageencodercontrol.h>
#include <qcameraimageprocessingcontrol.h>
#include <qmediaservice.h>
#include <qcamera.h>
#include <qcameraimagecapture.h>

#include "mockcameraservice.h"
#include "mockmediaserviceprovider.h"

QT_USE_NAMESPACE

class NullService: public QMediaService
{
    Q_OBJECT

public:
    NullService(): QMediaService(0)
    {

    }

    ~NullService()
    {

    }

    QMediaControl* requestControl(const char *iid)
    {
        Q_UNUSED(iid);
        return 0;
    }

    void releaseControl(QMediaControl*) {}

};

class tst_QCameraImageCapture: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void constructor();
    void mediaObject();
    void deleteMediaObject();
    void isReadyForCapture();
    void capture();
    void cancelCapture();
    void encodingSettings();
    void errors();
    void error();
    void imageCaptured();
    void imageExposed();
    void imageSaved();
    void readyForCaptureChanged();
    void supportedResolutions();
    void imageCodecDescription();
    void supportedImageCodecs();
    void cameraImageCaptureControl();

private:
    MockCameraService  *mockcameraservice;
    MockMediaServiceProvider *provider;
};

void tst_QCameraImageCapture::initTestCase()
{
    provider = new MockMediaServiceProvider;
    mockcameraservice = new MockCameraService;
    provider->service = mockcameraservice;
}

void tst_QCameraImageCapture::cleanupTestCase()
{
    delete mockcameraservice;
    delete provider;
}

//MaemoAPI-1823:test QCameraImageCapture Constructor
void tst_QCameraImageCapture::constructor()
{
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);
    QVERIFY(imageCapture.isAvailable() == true);
}

//MaemoAPI-1824:test mediaObject
void tst_QCameraImageCapture::mediaObject()
{
    MockMediaServiceProvider provider1;
    NullService  mymockcameraservice ;
    provider1.service = &mymockcameraservice;
    QCamera camera(0, &provider1);
    QCameraImageCapture imageCapture(&camera);
    QMediaObject *medobj = imageCapture.mediaObject();
    QVERIFY(medobj == NULL);

    QCamera camera1(0, provider);
    QCameraImageCapture imageCapture1(&camera1);
    QMediaObject *medobj1 = imageCapture1.mediaObject();
    QCOMPARE(medobj1, &camera1);
}

void tst_QCameraImageCapture::deleteMediaObject()
{
    MockMediaServiceProvider *provider = new MockMediaServiceProvider;
    provider->service = new MockCameraService;

    QCamera *camera = new QCamera(0, provider);
    QCameraImageCapture *capture = new QCameraImageCapture(camera);

    QVERIFY(capture->mediaObject() == camera);
    QVERIFY(capture->isAvailable());

    delete camera;
    delete provider->service;
    delete provider;

    //capture should detach from camera
    QVERIFY(capture->mediaObject() == 0);
    QVERIFY(!capture->isAvailable());

    capture->capture();
    delete capture;
}

//MaemoAPI-1825:test isReadyForCapture
void tst_QCameraImageCapture::isReadyForCapture()
{
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    camera.start();
    imageCapture.capture();
    QTest::qWait(300);
    QVERIFY(imageCapture.isReadyForCapture() == true);
    camera.stop();
}

//MaemoAPI-1826:test capture
void tst_QCameraImageCapture::capture()
{
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    QVERIFY(imageCapture.capture() == -1);
    camera.start();
    QVERIFY(imageCapture.isReadyForCapture() == true);
    QTest::qWait(300);
    QVERIFY(imageCapture.capture() != -1);
    camera.stop();
}

//MaemoAPI-1827:test cancelCapture
void tst_QCameraImageCapture::cancelCapture()
{
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);
    QSignalSpy spy(&imageCapture, SIGNAL(imageCaptured(int,QImage)));
    QSignalSpy spy1(&imageCapture, SIGNAL(imageSaved(int,QString)));
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    camera.start();
    imageCapture.capture();
    QTest::qWait(300);
    QVERIFY(imageCapture.isReadyForCapture() == true);
    QVERIFY(spy.count() == 1 && spy1.count() == 1);
    spy.clear();
    spy1.clear();
    camera.stop();

    QVERIFY(imageCapture.isReadyForCapture() == false);
    camera.start();
    imageCapture.capture();
    imageCapture.cancelCapture();
    QTest::qWait(300);
    QVERIFY(imageCapture.isReadyForCapture() == true);
    QVERIFY(spy.count() == 0 && spy1.count() == 0);
    camera.stop();
}

//MaemoAPI-1828:test encodingSettings
//MaemoAPI-1829:test set encodingSettings
void tst_QCameraImageCapture::encodingSettings()
{
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.encodingSettings() == QImageEncoderSettings());
    QImageEncoderSettings settings;
    settings.setCodec("JPEG");
    settings.setQuality(QtMultimedia::NormalQuality);
    imageCapture.setEncodingSettings(settings);
    QVERIFY(!imageCapture.encodingSettings().isNull());
    QVERIFY(imageCapture.encodingSettings().codec() == "JPEG");
    QVERIFY(imageCapture.encodingSettings().quality() == QtMultimedia::NormalQuality);
}

//MaemoAPI-1838:test supportedImageCodecs
void tst_QCameraImageCapture::supportedImageCodecs()
{
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(!imageCapture.supportedImageCodecs().isEmpty());
}

//MaemoAPI-1836:test supportedResolutions
void tst_QCameraImageCapture::supportedResolutions()
{
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.supportedResolutions().count() == 2);
    QImageEncoderSettings settings1;
    settings1.setCodec("PNG");;
    settings1.setResolution(320, 240);
    int result = imageCapture.supportedResolutions(settings1).count();
    QVERIFY(result == 1);
}

//MaemoAPI-1837:test imageCodecDescription
void tst_QCameraImageCapture::imageCodecDescription()
{
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.imageCodecDescription(" ").isNull());
    QVERIFY(imageCapture.imageCodecDescription("PNG").isNull() == false);
}

//MaemoAPI-1830:test errors
void tst_QCameraImageCapture::errors()
{
    MockMediaServiceProvider provider1 ;
    MockSimpleCameraService mockSimpleCameraService ;
    provider1.service = &mockSimpleCameraService;

    QCamera camera1(0, &provider1);
    QCameraImageCapture imageCapture1(&camera1);
    QVERIFY(imageCapture1.isAvailable() == false);
    imageCapture1.capture(QString::fromLatin1("/dev/null"));
    QVERIFY(imageCapture1.error() == QCameraImageCapture::NotSupportedFeatureError);
    QVERIFY2(!imageCapture1.errorString().isEmpty(), "Device does not support images capture");
    QVERIFY(imageCapture1.availabilityError() == QtMultimedia::ServiceMissingError);


    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.error() == QCameraImageCapture::NoError);
    QVERIFY(imageCapture.errorString().isEmpty());
    QVERIFY(imageCapture.availabilityError() == QtMultimedia::NoError);

    imageCapture.capture();
    QVERIFY(imageCapture.error() == QCameraImageCapture::NotReadyError);
    QVERIFY2(!imageCapture.errorString().isEmpty(), "Could not capture in stopped state");
    QVERIFY(imageCapture.availabilityError() == QtMultimedia::NoError);
}

//MaemoAPI-1831:test error
void tst_QCameraImageCapture::error()
{
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);
    QSignalSpy spy(&imageCapture, SIGNAL(error(int,QCameraImageCapture::Error,QString)));
    imageCapture.capture();
    QTest::qWait(30);
    QVERIFY(spy.count() == 1);
    QVERIFY(qvariant_cast<int>(spy.at(0).at(0)) == -1);
    QVERIFY(qvariant_cast<QCameraImageCapture::Error>(spy.at(0).at(1)) == QCameraImageCapture::NotReadyError);
    QVERIFY(qvariant_cast<QString>(spy.at(0).at(2)) == "Could not capture in stopped state");
    spy.clear();
}

//MaemoAPI-1832:test imageCaptured
void tst_QCameraImageCapture::imageCaptured()
{
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);
    QSignalSpy spy(&imageCapture, SIGNAL(imageCaptured(int,QImage)));
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    camera.start();
    imageCapture.capture();
    QTest::qWait(300);
    QVERIFY(imageCapture.isReadyForCapture() == true);

    QVERIFY(spy.count() == 1);
    QVERIFY(qvariant_cast<int>(spy.at(0).at(0)) > 0);
    QImage image = qvariant_cast<QImage>(spy.at(0).at(1));
    QVERIFY(image.isNull() == true);
    spy.clear();
    camera.stop();
}

//MaemoAPI-1833:test imageExposed
void tst_QCameraImageCapture::imageExposed()
{
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);
    QSignalSpy spy(&imageCapture, SIGNAL(imageExposed(int)));
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    camera.start();
    imageCapture.capture();
    QTest::qWait(300);
    QVERIFY(imageCapture.isReadyForCapture() == true);

    QVERIFY(spy.count() == 1);
    QVERIFY(qvariant_cast<int>(spy.at(0).at(0)) > 0);
    spy.clear();
    camera.stop();
}

//MaemoAPI-1834:test imageSaved
void tst_QCameraImageCapture::imageSaved()
{
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);
    QSignalSpy spy(&imageCapture, SIGNAL(imageSaved(int,QString)));
    QVERIFY(imageCapture.isAvailable() == true);
    QVERIFY(imageCapture.isReadyForCapture() == false);
    camera.start();
    imageCapture.capture(QString::fromLatin1("/usr/share"));
    QTest::qWait(300);
    QVERIFY(imageCapture.isReadyForCapture() == true);

    QVERIFY(spy.count() == 1);
    QVERIFY(qvariant_cast<int>(spy.at(0).at(0)) > 0);
    QVERIFY(qvariant_cast<QString>(spy.at(0).at(1)) == "/usr/share");
    spy.clear();
    camera.stop();
}

//MaemoAPI-1835:test readyForCaptureChanged
void tst_QCameraImageCapture::readyForCaptureChanged()
{
    QCamera camera(0, provider);
    QCameraImageCapture imageCapture(&camera);
    QSignalSpy spy(&imageCapture, SIGNAL(readyForCaptureChanged(bool)));
    QVERIFY(imageCapture.isReadyForCapture() == false);
    imageCapture.capture();
    QTest::qWait(100);
    QVERIFY(spy.count() == 0);
    QVERIFY2(!imageCapture.errorString().isEmpty(),"Could not capture in stopped state" );
    camera.start();
    QTest::qWait(100);
    imageCapture.capture();
    QTest::qWait(100);
    QVERIFY(spy.count() == 2);
    QVERIFY(spy.at(0).at(0).toBool() == false);
    QVERIFY(spy.at(1).at(0).toBool() == true);
    camera.stop();
    spy.clear();
}

//MaemoAPI-1853:test cameraImageCapture control constructor
void tst_QCameraImageCapture::cameraImageCaptureControl()
{
    MockCameraControl ctrl;
    MockCaptureControl capctrl(&ctrl);
}

QTEST_MAIN(tst_QCameraImageCapture)

#include "tst_qcameraimagecapture.moc"
