/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>

#include <qvideodevicecontrol.h>
#include <qmediacontrol.h>
#include <qmediaservice.h>

#include <QtGui/qapplication.h>
#include <QtGui/qstyle.h>

QT_BEGIN_NAMESPACE

class QtTestMediaService;

class tst_QMediaService : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();

    void control_iid();
    void control();
};


class QtTestMediaControlA : public QMediaControl
{
    Q_OBJECT
};

#define QtTestMediaControlA_iid "com.nokia.QtTestMediaControlA"
Q_MEDIA_DECLARE_CONTROL(QtTestMediaControlA, QtTestMediaControlA_iid)

class QtTestMediaControlB : public QMediaControl
{
    Q_OBJECT
};

#define QtTestMediaControlB_iid "com.nokia.QtTestMediaControlB"
Q_MEDIA_DECLARE_CONTROL(QtTestMediaControlB, QtTestMediaControlB_iid)


class QtTestMediaControlC : public QMediaControl
{
    Q_OBJECT
};

#define QtTestMediaControlC_iid "com.nokia.QtTestMediaControlC"
Q_MEDIA_DECLARE_CONTROL(QtTestMediaControlC, QtTestMediaControlA_iid) // Yes A.

class QtTestMediaControlD : public QMediaControl
{
    Q_OBJECT
};

#define QtTestMediaControlD_iid "com.nokia.QtTestMediaControlD"
Q_MEDIA_DECLARE_CONTROL(QtTestMediaControlD, QtTestMediaControlD_iid)

class QtTestMediaControlE : public QMediaControl
{
    Q_OBJECT
};

class QtTestMediaService : public QMediaService
{
    Q_OBJECT
public:
    QtTestMediaService()
        : QMediaService(0)
        , refA(0)
        , refB(0)
        , refC(0)
    {
    }

    QMediaControl *requestControl(const char *name)
    {
        if (strcmp(name, QtTestMediaControlA_iid) == 0) {
            refA += 1;

            return &controlA;
        } else if (strcmp(name, QtTestMediaControlB_iid) == 0) {
            refB += 1;

            return &controlB;
        } else if (strcmp(name, QtTestMediaControlC_iid) == 0) {
            refA += 1;

            return &controlA;
        } else {
            return 0;
        }
    }

    void releaseControl(QMediaControl *control)
    {
        if (control == &controlA)
            refA -= 1;
        else if (control == &controlB)
            refB -= 1;
        else if (control == &controlC)
            refC -= 1;
    }

    using QMediaService::requestControl;

    int refA;
    int refB;
    int refC;
    QtTestMediaControlA controlA;
    QtTestMediaControlB controlB;
    QtTestMediaControlC controlC;
};

void tst_QMediaService::initTestCase()
{    
}

void tst_QMediaService::control_iid()
{
    const char *nullString = 0;

    // Default implementation.
    QCOMPARE(qmediacontrol_iid<QtTestMediaControlE *>(), nullString);

    // Partial template.
    QVERIFY(qstrcmp(qmediacontrol_iid<QtTestMediaControlA *>(), QtTestMediaControlA_iid) == 0);
}

void tst_QMediaService::control()
{
    QtTestMediaService service;

    QtTestMediaControlA *controlA = service.requestControl<QtTestMediaControlA *>();
    QCOMPARE(controlA, &service.controlA);
    service.releaseControl(controlA);

    QtTestMediaControlB *controlB = service.requestControl<QtTestMediaControlB *>();
    QCOMPARE(controlB, &service.controlB);
    service.releaseControl(controlB);

    QVERIFY(!service.requestControl<QtTestMediaControlC *>());  // Faulty implementation returns A.
    QCOMPARE(service.refA, 0);  // Verify the control was released.

    QVERIFY(!service.requestControl<QtTestMediaControlD *>());  // No control of that type.
}

QT_END_NAMESPACE

QT_USE_NAMESPACE

QTEST_MAIN(tst_QMediaService)

#include "tst_qmediaservice.moc"
