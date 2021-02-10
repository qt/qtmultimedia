/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtMultimedia/qmediametadata.h>
#include <qmediasource.h>
#include <qmediaservice.h>

#include "mockmediarecorderservice.h"

class QtTestMediaObjectService : public QMediaService
{
    Q_OBJECT
public:
    QtTestMediaObjectService(QObject *parent = nullptr)
        : QMediaService(parent)
    {
    }

    QObject *requestControl(const char *) override
    {
        return nullptr;
    }

    void releaseControl(QObject *) override
    {
    }
};

QT_USE_NAMESPACE

class tst_QMediaSource : public QObject
{
    Q_OBJECT

private slots:
    void availability();
    void service();
};
class QtTestMediaObject : public QMediaSource
{
    Q_OBJECT

public:
    QtTestMediaObject(QMediaService *service = nullptr): QMediaSource(nullptr, service) {}
};

void tst_QMediaSource::availability()
{
    {
        QtTestMediaObject nullObject(nullptr);
        QCOMPARE(nullObject.isAvailable(), false);
        QCOMPARE(nullObject.availability(), QMultimedia::ServiceMissing);
    }

    {
        QtTestMediaObjectService service;
        QtTestMediaObject object(&service);
        QCOMPARE(object.isAvailable(), true);
        QCOMPARE(object.availability(), QMultimedia::Available);
    }
}

 void tst_QMediaSource::service()
 {
     // Create the mediaobject with service.
     QtTestMediaObjectService service;
     QtTestMediaObject mediaObject1(&service);

     // Get service and Compare if it equal to the service passed as an argument in mediaObject1.
     QMediaService *service1 = mediaObject1.service();
     QVERIFY(service1 != nullptr);
     QCOMPARE(service1,&service);

     // Create the mediaobject with empty service and verify that service() returns NULL.
     QtTestMediaObject mediaObject2;
     QMediaService *service2 = mediaObject2.service();
     QVERIFY(service2 == nullptr);
}

QTEST_GUILESS_MAIN(tst_QMediaSource)
#include "tst_qmediasource.moc"
