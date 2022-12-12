// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicksoundeffect_p.h"
#include <QtQml/qqmlcontext.h>

QT_BEGIN_NAMESPACE

QQuickSoundEffect::QQuickSoundEffect(QObject *parent) : QSoundEffect(parent) { }

void QQuickSoundEffect::qmlSetSource(const QUrl &source)
{
    if (m_source == source)
        return;

    m_source = source;
    const QQmlContext *context = qmlContext(this);
    setSource(context ? context->resolvedUrl(source) : source);
    emit sourceChanged(source);
}

QUrl QQuickSoundEffect::qmlSource() const
{
    return m_source;
}

QT_END_NAMESPACE

#include "moc_qquicksoundeffect_p.cpp"
