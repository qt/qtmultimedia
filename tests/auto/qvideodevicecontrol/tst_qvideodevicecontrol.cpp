/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include "qvideodevicecontrol.h"
class TestClass: public QVideoDeviceControl
{
    Q_OBJECT

public:
    TestClass(QObject *parent = 0 ):QVideoDeviceControl(parent)
    {

    }

    ~TestClass(){}

    virtual int deviceCount() const { return 0; }

    QString deviceName(int index) const
    {
        QString str;
        return str;
    }

    QString deviceDescription(int index) const
    {
        QString str;
        return str;
    }

    QIcon deviceIcon(int index) const
    {
        QIcon icon;
        return icon;
    }

    int defaultDevice() const { return 0; }
    int selectedDevice() const { return 0; }

 public Q_SLOTS:
    void setSelectedDevice(int index)
    {
        emit devicesChanged();
        emit selectedDeviceChanged(index);
        emit selectedDeviceChanged("abc");
    }
};


class tst_QVideoDeviceControl : public QObject
{
    Q_OBJECT
public:
    tst_QVideoDeviceControl(){}
    ~tst_QVideoDeviceControl(){}

private slots:
    void testQVideoDeviceControl();
};

//MaemoAPI-1859:QVideoDeviceControl constructor
void  tst_QVideoDeviceControl::testQVideoDeviceControl()
{
    TestClass *testClass = new TestClass(this);
    QVERIFY(testClass != NULL);
}

QTEST_MAIN(tst_QVideoDeviceControl)
#include "tst_qvideodevicecontrol.moc"
