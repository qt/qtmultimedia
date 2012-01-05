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


#include <QtTest/QtTest>
#include <QtCore/QString>
#include <QSound>

class tst_QSound : public QObject
{
   Q_OBJECT

public:
   tst_QSound( QObject* parent=0) : QObject(parent) {}

private slots:
   void initTestCase();
   void cleanupTestCase();
   void testLooping();
   void testPlay();
   void testStop();

   void testStaticPlay();

private:
    QSound* sound;
};


void tst_QSound::initTestCase()
{
    const QString testFileName = QStringLiteral("test.wav");
    const QString fullPath = QFINDTESTDATA(testFileName);
    QVERIFY2(!fullPath.isEmpty(), qPrintable(QStringLiteral("Unable to locate ") + testFileName));
    sound = new QSound(fullPath, this);

    QVERIFY(!sound->fileName().isEmpty());
    QCOMPARE(sound->loops(),1);
}

void tst_QSound::cleanupTestCase()
{
    if (sound)
    {
        delete sound;
        sound = NULL;
    }
}

void tst_QSound::testLooping()
{
    sound->setLoops(5);
    QCOMPARE(sound->loops(),5);

    sound->play();
    QVERIFY(!sound->isFinished());

    // test.wav is about 200ms, wait until it has finished playing 5 times
    QTest::qWait(3000);

    QVERIFY(sound->isFinished());
    QCOMPARE(sound->loopsRemaining(),0);
}

void tst_QSound::testPlay()
{
    sound->setLoops(1);
    sound->play();
    QVERIFY(!sound->isFinished());
    QTest::qWait(1000);
    QVERIFY(sound->isFinished());
}

void tst_QSound::testStop()
{
    sound->setLoops(10);
    sound->play();
    QVERIFY(!sound->isFinished());
    QTest::qWait(1000);
    sound->stop();
    QTest::qWait(1000);
    QVERIFY(sound->isFinished());
}

void tst_QSound::testStaticPlay()
{
    // Check that you hear sound with static play also.
    const QString testFileName = QStringLiteral("test2.wav");
    const QString fullPath = QFINDTESTDATA(testFileName);
    QVERIFY2(!fullPath.isEmpty(), qPrintable(QStringLiteral("Unable to locate ") + testFileName));

    QSound::play(fullPath);

    QTest::qWait(1000);
}

QTEST_MAIN(tst_QSound);
#include "tst_qsound.moc"
