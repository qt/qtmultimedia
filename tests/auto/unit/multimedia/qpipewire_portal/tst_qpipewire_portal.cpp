// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <private/qpipewire_portal_p.h>

#include <QtDBus/qdbusconnection.h>

using namespace QtPipeWire;
using namespace Qt::StringLiterals;

class tst_QPipeWirePortal : public QObject
{
    Q_OBJECT

private slots:
    void makePortalRequestToken_returnsDistinctTokens();
    void portalRequestPath_derivesPathFromTheUniqueName_data();
    void portalRequestPath_derivesPathFromTheUniqueName();
    void portalRequestPath_matchesTheSessionBusName();
    void portalInterfaceVersion_returnsNullopt_whenInterfaceDoesNotExist();
};

void tst_QPipeWirePortal::makePortalRequestToken_returnsDistinctTokens()
{
    const QString first = allocatePortalRequestToken();
    const QString second = allocatePortalRequestToken();

    QVERIFY(!first.isEmpty());
    QCOMPARE_NE(first, second);

    // Has to be usable as a dbus object path element.
    for (const QString &token : { first, second }) {
        for (QChar c : token)
            QVERIFY2(c.isLetterOrNumber() || c == u'_', qPrintable(token));
    }
}

void tst_QPipeWirePortal::portalRequestPath_derivesPathFromTheUniqueName_data()
{
    QTest::addColumn<QString>("uniqueName");
    QTest::addColumn<QString>("expected");

    // The portal drops the leading colon and turns every dot into an
    // underscore.
    QTest::newRow("typical unique name")
            << u":1.234"_s << u"/org/freedesktop/portal/desktop/request/1_234/tok"_s;
    QTest::newRow("multiple dots")
            << u":1.2.3"_s << u"/org/freedesktop/portal/desktop/request/1_2_3/tok"_s;
    QTest::newRow("no leading colon")
            << u"1.234"_s << u"/org/freedesktop/portal/desktop/request/1_234/tok"_s;
}

void tst_QPipeWirePortal::portalRequestPath_derivesPathFromTheUniqueName()
{
    QFETCH(const QString, uniqueName);
    QFETCH(const QString, expected);

    QCOMPARE(portalRequestPath(uniqueName, u"tok"_s), expected);
}

void tst_QPipeWirePortal::portalRequestPath_matchesTheSessionBusName()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        QSKIP("No session bus available");

    const QString path = portalRequestPath(bus, u"tok"_s);
    QVERIFY(path.startsWith(u"/org/freedesktop/portal/desktop/request/"_s));
    QVERIFY(path.endsWith(u"/tok"_s));
    QVERIFY(!path.contains(u':'));
    QCOMPARE(portalRequestPath(bus.baseService(), u"tok"_s), path);
}

void tst_QPipeWirePortal::portalInterfaceVersion_returnsNullopt_whenInterfaceDoesNotExist()
{
    if (!QDBusConnection::sessionBus().isConnected())
        QSKIP("No session bus available");

    QCOMPARE(portalInterfaceVersion("org.freedesktop.portal.NoSuchInterface"_L1), std::nullopt);
}

QTEST_GUILESS_MAIN(tst_QPipeWirePortal)

#include "tst_qpipewire_portal.moc"
