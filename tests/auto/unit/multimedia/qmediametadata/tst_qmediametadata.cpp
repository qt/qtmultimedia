// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtCore/qdebug.h>
#include <QtCore/qset.h>
#include <QtCore/qstring.h>

#include <QtMultimedia/qmediametadata.h>

QT_USE_NAMESPACE

using namespace Qt::Literals;

class tst_QMediaMetaData : public QObject
{
    Q_OBJECT

private slots:
    void insertAndRemove();
    void qdebug_empty();
    void qdebug_printContent();
};

void tst_QMediaMetaData::insertAndRemove()
{
    QMediaMetaData dut;
    QVERIFY(dut.isEmpty());

    // fill
    dut.insert(QMediaMetaData::Author, "yada");
    dut.insert(QMediaMetaData::Title, "title");
    QVERIFY(!dut.isEmpty());

    // validate
    {
        auto range = dut.asKeyValueRange();
        QCOMPARE_EQ(std::distance(range.begin(), range.end()), 2u);

        QSet expectedKeys{
            QMediaMetaData::Author,
            QMediaMetaData::Title,
        };

        QList keyList = dut.keys();
        QSet<QMediaMetaData::Key> keys{ keyList.begin(), keyList.end() };

        QCOMPARE_EQ(keys, expectedKeys);

        QCOMPARE(dut.value(QMediaMetaData::Author), u"yada"_s);
        QCOMPARE(dut.stringValue(QMediaMetaData::Author), u"yada"_s);
    };

    // remove missing key
    QMediaMetaData reference = dut;
    dut.remove(QMediaMetaData::AlbumArtist);
    QCOMPARE_EQ(dut, reference);

    // clear
    dut.clear();
    QVERIFY(dut.isEmpty());
}

void tst_QMediaMetaData::qdebug_empty()
{
    QMediaMetaData dut;

    QString str;
    QDebug dbg(&str);
    dbg << dut;

    auto expected = u"QMediaMetaData{} ";

    QCOMPARE_EQ(str, expected);
}

void tst_QMediaMetaData::qdebug_printContent()
{
    QMediaMetaData dut;
    dut.insert(QMediaMetaData::Author, "yada");
    dut.insert(QMediaMetaData::Title, "title");

    QString str;
    QDebug dbg(&str);
    dbg << dut;

    auto expected = u"QMediaMetaData{QMediaMetaData::Title: QVariant(QString, \"title\"), "
                    u"QMediaMetaData::Author: QVariant(QString, \"yada\")} ";
    auto expected2 = u"QMediaMetaData{QMediaMetaData::Author: QVariant(QString, \"yada\"), "
                     u"QMediaMetaData::Title: QVariant(QString, \"title\")} ";

    QVERIFY(str == expected || str == expected2);
}

QTEST_GUILESS_MAIN(tst_QMediaMetaData)
#include "tst_qmediametadata.moc"
