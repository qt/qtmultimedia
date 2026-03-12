// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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

#ifndef QMOCKRESOLVERS_P_H
#define QMOCKRESOLVERS_P_H

#include <QtCore/qurl.h>
#include <QtMultimedia/private/qmultimedia_source_resolver_p.h>

namespace QtMultimediaTest {

class MockSourceResolver : public QMultimediaPrivate::AbstractSourceResolver
{
public:
    explicit MockSourceResolver(const QUrl &ret = QUrl()) : m_return(ret) { }
    QUrl resolve(QUrl in) const override
    {
        ++m_callCount;
        m_lastInput = in;
        if (!m_return.isEmpty())
            return m_return;
        return in;
    }

    mutable int m_callCount{};
    mutable QUrl m_lastInput;
    QUrl m_return;
};

} // namespace QtMultimediaTest

#endif // QMOCKRESOLVERS_P_H
