// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
class tst_multimedia_static_linking : public QObject
{
    Q_OBJECT

private slots:
    void dummy()
    {}
};

QTEST_MAIN(tst_multimedia_static_linking)

#include "tst_multimedia_static_linking.moc"
