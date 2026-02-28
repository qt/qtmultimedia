// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMULTIMEDIA_SOURCE_RESOLVER_P_H
#define QMULTIMEDIA_SOURCE_RESOLVER_P_H

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

#include <QtMultimedia/qtmultimediaglobal.h>

QT_BEGIN_NAMESPACE

class QUrl;

namespace QMultimediaPrivate {

class Q_MULTIMEDIA_EXPORT AbstractSourceResolver
{
public:
    virtual ~AbstractSourceResolver();
    virtual QUrl resolve(QUrl) const = 0;
};

class Q_MULTIMEDIA_EXPORT TrivialSourceResolver final : public AbstractSourceResolver
{
public:
    ~TrivialSourceResolver();
    QUrl resolve(QUrl) const override;
};

} // namespace QMultimediaPrivate

QT_END_NAMESPACE

#endif // QMULTIMEDIA_SOURCE_RESOLVER_P_H
