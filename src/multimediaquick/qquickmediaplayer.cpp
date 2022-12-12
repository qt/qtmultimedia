// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmediaplayer_p.h"
#include <QtQml/qqmlcontext.h>

QT_BEGIN_NAMESPACE

QQuickMediaPlayer::QQuickMediaPlayer(QObject *parent) : QMediaPlayer(parent) { }

void QQuickMediaPlayer::qmlSetSource(const QUrl &source)
{
    if (m_source == source)
        return;
    m_source = source;
    const QQmlContext *context = qmlContext(this);
    setSource(context ? context->resolvedUrl(source) : source);
    emit sourceChanged(source);
}

QUrl QQuickMediaPlayer::qmlSource() const
{
    return m_source;
}
QT_END_NAMESPACE

#include "moc_qquickmediaplayer_p.cpp"
