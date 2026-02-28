// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlcontext_source_resolver_p.h"

#include <QtQml/qqml.h>
#include <QtQml/qqmlcontext.h>

QT_BEGIN_NAMESPACE

namespace QMultimediaPrivate {

QQmlContextSourceResolver::QQmlContextSourceResolver(QObject *object) : m_objectForContext(object)
{
}

QUrl QQmlContextSourceResolver::resolve(QUrl url) const
{
    const QQmlContext *context = qmlContext(m_objectForContext);
    return context ? context->resolvedUrl(url) : url;
}

} // namespace QMultimediaPrivate

QT_END_NAMESPACE
