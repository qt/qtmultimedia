// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmediaplayer_p.h"
#include <QtQml/qqmlcontext.h>

QT_BEGIN_NAMESPACE

QQuickMediaPlayer::QQuickMediaPlayer(QObject *parent) : QMediaPlayer(parent)
{
    connect(this, &QMediaPlayer::positionChanged, this, &QQuickMediaPlayer::onPositionChanged);
    connect(this, &QMediaPlayer::durationChanged, this, &QQuickMediaPlayer::onDurationChanged);
}

void QQuickMediaPlayer::qmlSetSource(const QUrl &source)
{
    if (m_source == source)
        return;
    m_source = source;
    const QQmlContext *context = qmlContext(this);
    setSource(context ? context->resolvedUrl(source) : source);
    emit qmlSourceChanged(source);
}

QUrl QQuickMediaPlayer::qmlSource() const
{
    return m_source;
}

void QQuickMediaPlayer::setQmlPosition(int position)
{
    setPosition(static_cast<qint64>(position));
}

int QQuickMediaPlayer::qmlPosition() const
{
    return static_cast<int>(position());
}

int QQuickMediaPlayer::qmlDuration() const
{
    return static_cast<int>(duration());
}

void QQuickMediaPlayer::onPositionChanged(qint64 position)
{
    emit qmlPositionChanged(static_cast<int>(position));
}

void QQuickMediaPlayer::onDurationChanged(qint64 duration)
{
    emit qmlDurationChanged(static_cast<int>(duration));
}

QT_END_NAMESPACE

#include "moc_qquickmediaplayer_p.cpp"
