// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLCONTEXT_SOURCE_RESOLVER_P_H
#define QQMLCONTEXT_SOURCE_RESOLVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimediaQuick/private/qtmultimediaquickglobal_p.h>
#include <QtMultimedia/private/qmultimedia_source_resolver_p.h>

QT_BEGIN_NAMESPACE

namespace QMultimediaPrivate {

class Q_MULTIMEDIAQUICK_EXPORT QQmlContextSourceResolver final : public AbstractSourceResolver
{
public:
    explicit QQmlContextSourceResolver(QObject *objectForContext);
    QUrl resolve(QUrl) const override;

protected:
    const QObject *const m_objectForContext;
};

} // namespace QMultimediaPrivate

QT_END_NAMESPACE

#endif
