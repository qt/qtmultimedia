// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
