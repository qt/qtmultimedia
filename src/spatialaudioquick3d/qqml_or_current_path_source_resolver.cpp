// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#include "qqml_or_current_path_source_resolver_p.h"

#include <QtCore/qurl.h>
#include <QtCore/qdir.h>

QT_BEGIN_NAMESPACE

namespace QMultimediaPrivate {

QQmlContextOrCurrentPathSourceResolver::QQmlContextOrCurrentPathSourceResolver(
        QObject *objectForContext)
    : QQmlContextSourceResolver(objectForContext)
{
}

QUrl QQmlContextOrCurrentPathSourceResolver::resolve(QUrl url) const
{
    if (!m_objectForContext)
        return QUrl::fromLocalFile(QDir::currentPath() + u"/").resolved(url);
    return QQmlContextSourceResolver::resolve(url);
}

}; // namespace QMultimediaPrivate

QT_END_NAMESPACE
