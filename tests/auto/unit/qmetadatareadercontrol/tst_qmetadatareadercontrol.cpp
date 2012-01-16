/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
#include <QtCore/QString>
#include <QtTest/QtTest>
#include <QtCore/QCoreApplication>

#include "mockmetadatareadercontrol.h"

class tst_QMetaDataReaderControl : public QObject
{
    Q_OBJECT

private slots:
    // Test case for QMetaDataReaderControl
    void metaDataReaderControlConstructor();
    void metaDataReaderControlAvailableMetaData();
    void metaDataReaderControlIsMetaDataAvailable();
    void metaDataReaderControlMetaData();
    void metaDataReaderControlMetaDataAvailableChangedSignal();
    void metaDataReaderControlMetaDataChangedSignal();
};

QTEST_MAIN(tst_QMetaDataReaderControl);

/* Test case for constructor. */
void tst_QMetaDataReaderControl::metaDataReaderControlConstructor()
{
    MockMetaDataReaderControl *metaData = new MockMetaDataReaderControl();
    QVERIFY(metaData !=NULL);
    delete metaData;
}

/* Test case for availableMetaData() */
void tst_QMetaDataReaderControl::metaDataReaderControlAvailableMetaData()
{
    MockMetaDataReaderControl *metaData = new MockMetaDataReaderControl();
    QVERIFY(metaData !=NULL);
    metaData->availableMetaData() ;
    delete metaData;
}

/* Test case for availableMetaData */
void tst_QMetaDataReaderControl::metaDataReaderControlIsMetaDataAvailable ()
{
    MockMetaDataReaderControl *metaData = new MockMetaDataReaderControl();
    QVERIFY(metaData !=NULL);
    metaData->availableMetaData();
    delete metaData;
}

/* Test case for metaData */
void tst_QMetaDataReaderControl::metaDataReaderControlMetaData ()
{
    MockMetaDataReaderControl *metaData = new MockMetaDataReaderControl();
    QVERIFY(metaData !=NULL);
    metaData->metaData(QtMultimedia::MetaData::Title);
    delete metaData;
}

/* Test case for signal metaDataAvailableChanged */
void tst_QMetaDataReaderControl::metaDataReaderControlMetaDataAvailableChangedSignal ()
{
    MockMetaDataReaderControl *metaData = new MockMetaDataReaderControl();
    QVERIFY(metaData !=NULL);
    QSignalSpy spy(metaData,SIGNAL(metaDataAvailableChanged(bool)));
    metaData->setMetaDataAvailable(true);
    QVERIFY(spy.count() == 1);
    delete metaData;
}

 /* Test case for signal metaDataChanged */
void tst_QMetaDataReaderControl::metaDataReaderControlMetaDataChangedSignal ()
{
    MockMetaDataReaderControl *metaData = new MockMetaDataReaderControl();
    QVERIFY(metaData !=NULL);
    QSignalSpy spy(metaData,SIGNAL(metaDataChanged()));
    metaData->metaDataChanged();
    QVERIFY(spy.count () == 1);
    delete metaData;
}

#include "tst_qmetadatareadercontrol.moc"


