// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmultimedia_source_resolver_p.h"

#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

namespace QMultimediaPrivate {

AbstractSourceResolver::~AbstractSourceResolver() = default;

TrivialSourceResolver::~TrivialSourceResolver() = default;

QUrl TrivialSourceResolver::resolve(QUrl source) const
{
    return source;
}

} // namespace QMultimediaPrivate

QT_END_NAMESPACE
