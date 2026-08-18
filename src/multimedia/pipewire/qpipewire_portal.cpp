// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_portal_p.h"

#include <QtGui/private/qdesktopunixservices_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/quuid.h>
#include <QtDBus/qdbusconnection.h>
#include <QtDBus/qdbusinterface.h>
#include <QtDBus/qdbusmessage.h>

#include <atomic>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(lcPipewirePortal, "qt.multimedia.pipewire.portal")

std::optional<unsigned int> portalInterfaceVersion(QLatin1StringView interface)
{
    QDBusInterface properties(u"org.freedesktop.portal.Desktop"_s,
                              u"/org/freedesktop/portal/desktop"_s,
                              u"org.freedesktop.DBus.Properties"_s, QDBusConnection::sessionBus());

    QDBusMessage reply = properties.call(u"Get"_s, QString{ interface }, u"version"_s);

    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().size() != 1) {
        qCDebug(lcPipewirePortal) << interface << "not available:" << reply.errorName()
                                  << reply.errorMessage();
        return std::nullopt;
    }

    // The property is a variant, which arrives wrapped in a QDBusVariant.
    const uint version = reply.arguments().at(0).toUInt();
    qCDebug(lcPipewirePortal) << interface << "version" << version;
    return version;
}

QString allocatePortalRequestToken()
{
    static const QString prefix =
            u"qtmm%1_"_s.arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
    static std::atomic_int counter;

    return prefix + QString::number(counter.fetch_add(1, std::memory_order_relaxed));
}

QString portalRequestPath(const QString &uniqueName, const QString &token)
{
    // The portal derives the path from the caller's unique name with the
    // leading ':' dropped and every '.' turned into '_'.
    QString sender = uniqueName;
    if (sender.startsWith(u':'))
        sender.remove(0, 1);
    sender.replace(u'.', u'_');

    return u"/org/freedesktop/portal/desktop/request/%1/%2"_s.arg(sender, token);
}

QString portalRequestPath(const QDBusConnection &connection, const QString &token)
{
    return portalRequestPath(connection.baseService(), token);
}

QString portalParentWindowIdentifier()
{
    auto *unixServices = dynamic_cast<QDesktopUnixServices *>(
            QGuiApplicationPrivate::platformIntegration()->services());
    QWindow *focusWindow = QGuiApplication::focusWindow();
    if (!unixServices || !focusWindow)
        return {};

    return unixServices->portalWindowIdentifier(focusWindow);
}

} // namespace QtPipeWire

QT_END_NAMESPACE
