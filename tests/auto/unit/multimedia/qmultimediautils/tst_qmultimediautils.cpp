/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <QtTest/QtTest>
#include <QDebug>
#include <private/qmultimediautils_p.h>

class tst_QMultimediaUtils : public QObject
{
    Q_OBJECT

private slots:
    void fraction_of_0();
    void fraction_of_negative_1_5();
    void fraction_of_1_5();
    void fraction_of_30();
    void fraction_of_29_97();
    void fraction_of_lower_boundary();
    void fraction_of_upper_boundary();
};

void tst_QMultimediaUtils::fraction_of_0()
{
    auto [n, d] = qRealToFraction(0.);
    QCOMPARE(n, 0);
    QCOMPARE(d, 1);
}

void tst_QMultimediaUtils::fraction_of_negative_1_5()
{
    auto [n, d] = qRealToFraction(-1.5);
    QCOMPARE(double(n) / double(d), -1.5);
    QCOMPARE(n, -3);
    QCOMPARE(d, 2);
}

void tst_QMultimediaUtils::fraction_of_1_5()
{
    auto [n, d] = qRealToFraction(1.5);
    QCOMPARE(double(n) / double(d), 1.5);
    QCOMPARE(n, 3);
    QCOMPARE(d, 2);
}

void tst_QMultimediaUtils::fraction_of_30()
{
    auto [n, d] = qRealToFraction(30.);
    QCOMPARE(double(n) / double(d), 30.);
    QCOMPARE(d, 1);
}

void tst_QMultimediaUtils::fraction_of_29_97()
{
    auto [n, d] = qRealToFraction(29.97);
    QCOMPARE(double(n) / double(d), 29.97);
}

void tst_QMultimediaUtils::fraction_of_lower_boundary()
{
    double f = 0.000001;
    auto [n, d] = qRealToFraction(f);
    QVERIFY(double(n) / double(d) < f);
    QVERIFY(double(n) / double(d) >= 0.);
}

void tst_QMultimediaUtils::fraction_of_upper_boundary()
{
    double f = 0.999999;
    auto [n, d] = qRealToFraction(f);
    QVERIFY(double(n) / double(d) <= 1.);
    QVERIFY(double(n) / double(d) > f);
}

QTEST_MAIN(tst_QMultimediaUtils)
#include "tst_qmultimediautils.moc"
