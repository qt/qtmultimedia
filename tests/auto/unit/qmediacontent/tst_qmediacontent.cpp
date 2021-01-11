/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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
#include <QtNetwork/qnetworkrequest.h>

#include <QUrl.h>
#include <qmediaplaylist.h>

//TESTED_COMPONENT=src/multimedia

QT_USE_NAMESPACE
class tst_QUrl : public QObject
{
    Q_OBJECT

private slots:
    void testNull();
    void testUrlCtor();
    void testRequestCtor();
    void testCopy();
    void testAssignment();
    void testEquality();
    void testPlaylist();
};

void tst_QUrl::testNull()
{
    QUrl media;

    QCOMPARE(media.isNull(), true);
    QCOMPARE(media.request().url(), QUrl());
}

void tst_QUrl::testUrlCtor()
{
    QUrl media(QUrl("http://example.com/movie.mov"));
    QCOMPARE(media.request().url(), QUrl("http://example.com/movie.mov"));
}

void tst_QUrl::testRequestCtor()
{
    QNetworkRequest request(QUrl("http://example.com/movie.mov"));
    request.setAttribute(QNetworkRequest::User, QVariant(1234));

    QUrl media(request);
    QCOMPARE(media.request().url(), QUrl("http://example.com/movie.mov"));
    QCOMPARE(media.request(), request);
}

void tst_QUrl::testCopy()
{
    QUrl media1(QUrl("http://example.com/movie.mov"));
    QUrl media2(media1);

    QVERIFY(media1 == media2);
}

void tst_QUrl::testAssignment()
{
    QUrl media1(QUrl("http://example.com/movie.mov"));
    QUrl media2;
    QUrl media3;

    media2 = media1;
    QVERIFY(media2 == media1);

    media2 = media3;
    QVERIFY(media2 == media3);
}

void tst_QUrl::testEquality()
{
    QUrl media1;
    QUrl media2;
    QUrl media3(QUrl("http://example.com/movie.mov"));
    QUrl media4(QUrl("http://example.com/movie.mov"));
    QUrl media5(QUrl("file:///some/where/over/the/rainbow.mp3"));

    // null == null
    QCOMPARE(media1 == media2, true);
    QCOMPARE(media1 != media2, false);

    // null != something
    QCOMPARE(media1 == media3, false);
    QCOMPARE(media1 != media3, true);

    // equiv
    QCOMPARE(media3 == media4, true);
    QCOMPARE(media3 != media4, false);

    // not equiv
    QCOMPARE(media4 == media5, false);
    QCOMPARE(media4 != media5, true);
}

void tst_QUrl::testPlaylist()
{
    QUrl media(QUrl("http://example.com/movie.mov"));
    QVERIFY(media.request().url().isValid());
    QVERIFY(!media.playlist());

    {
        QPointer<QMediaPlaylist> playlist(new QMediaPlaylist);
        media = QUrl(playlist.data(), QUrl("http://example.com/sample.m3u"), true);
        QVERIFY(media.request().url().isValid());
        QCOMPARE(media.playlist(), playlist.data());
        media = QUrl();
        // Make sure playlist is destroyed by QUrl
        QTRY_VERIFY(!playlist);
    }

    {
        QMediaPlaylist *playlist = new QMediaPlaylist;
        media = QUrl(playlist, QUrl("http://example.com/sample.m3u"), true);
        // Delete playlist outside QUrl
        delete playlist;
        QVERIFY(!media.playlist());
        media = QUrl();
    }

    {
        QPointer<QMediaPlaylist> playlist(new QMediaPlaylist);
        media = QUrl(playlist.data(), QUrl(), false);
        QVERIFY(!media.request().url().isValid());
        QCOMPARE(media.playlist(), playlist.data());
        media = QUrl();
        QVERIFY(playlist);
        delete playlist.data();
    }
}

QTEST_MAIN(tst_QUrl)

#include "tst_QUrl.moc"
