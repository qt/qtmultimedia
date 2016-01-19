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

#include "qaudio.h"

// Adds an enum, and the stringized version
#define ADD_ENUM_TEST(x) \
    QTest::newRow(#x) \
        << QAudio::x \
    << QString(QLatin1String(#x));

class tst_QAudioNamespace : public QObject
{
    Q_OBJECT

private slots:
    void debugError();
    void debugError_data();
    void debugState();
    void debugState_data();
    void debugMode();
    void debugMode_data();
};

void tst_QAudioNamespace::debugError_data()
{
    QTest::addColumn<QAudio::Error>("error");
    QTest::addColumn<QString>("stringized");

    ADD_ENUM_TEST(NoError);
    ADD_ENUM_TEST(OpenError);
    ADD_ENUM_TEST(IOError);
    ADD_ENUM_TEST(UnderrunError);
    ADD_ENUM_TEST(FatalError);
}

void tst_QAudioNamespace::debugError()
{
    QFETCH(QAudio::Error, error);
    QFETCH(QString, stringized);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << error;
}

void tst_QAudioNamespace::debugState_data()
{
    QTest::addColumn<QAudio::State>("state");
    QTest::addColumn<QString>("stringized");

    ADD_ENUM_TEST(ActiveState);
    ADD_ENUM_TEST(SuspendedState);
    ADD_ENUM_TEST(StoppedState);
    ADD_ENUM_TEST(IdleState);
}

void tst_QAudioNamespace::debugState()
{
    QFETCH(QAudio::State, state);
    QFETCH(QString, stringized);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << state;
}

void tst_QAudioNamespace::debugMode_data()
{
    QTest::addColumn<QAudio::Mode>("mode");
    QTest::addColumn<QString>("stringized");

    ADD_ENUM_TEST(AudioInput);
    ADD_ENUM_TEST(AudioOutput);
}

void tst_QAudioNamespace::debugMode()
{
    QFETCH(QAudio::Mode, mode);
    QFETCH(QString, stringized);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << mode;
}
QTEST_MAIN(tst_QAudioNamespace)

#include "tst_qaudionamespace.moc"
