// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_PORTAL_P_H
#define QPIPEWIRE_PORTAL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtCore/qlatin1stringview.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringview.h>

#include <optional>

QT_BEGIN_NAMESPACE

class QDBusConnection;

namespace QtPipeWire {

using namespace Qt::Literals::StringLiterals;

Q_MULTIMEDIA_EXPORT std::optional<unsigned> portalInterfaceVersion(QLatin1StringView interface);

Q_MULTIMEDIA_EXPORT QString allocatePortalRequestToken();
Q_MULTIMEDIA_EXPORT QString portalRequestPath(const QString &uniqueName, const QString &token);
Q_MULTIMEDIA_EXPORT QString portalRequestPath(const QDBusConnection &, const QString &token);
Q_MULTIMEDIA_EXPORT QString portalParentWindowIdentifier();

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_PORTAL_P_H
