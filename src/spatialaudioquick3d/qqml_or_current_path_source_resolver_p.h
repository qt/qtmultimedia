// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#ifndef QQML_OR_CURRENT_PATH_SOURCE_RESOLVER_P_H
#define QQML_OR_CURRENT_PATH_SOURCE_RESOLVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimediaQuick/private/qqmlcontext_source_resolver_p.h>

QT_BEGIN_NAMESPACE

namespace QMultimediaPrivate {

class QQmlContextOrCurrentPathSourceResolver final : public QQmlContextSourceResolver
{
public:
    explicit QQmlContextOrCurrentPathSourceResolver(QObject *objectForContext);
    QUrl resolve(QUrl) const override;
};

}; // namespace QMultimediaPrivate

QT_END_NAMESPACE

#endif // QQML_OR_CURRENT_PATH_SOURCE_RESOLVER_P_H
