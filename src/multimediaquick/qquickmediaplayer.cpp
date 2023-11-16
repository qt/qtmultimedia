// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmediaplayer_p.h"
#include <QtQml/qqmlcontext.h>

QT_BEGIN_NAMESPACE

QQuickMediaPlayer::QQuickMediaPlayer(QObject *parent) : QMediaPlayer(parent)
{
    connect(this, &QMediaPlayer::positionChanged, this, &QQuickMediaPlayer::onPositionChanged);
    connect(this, &QMediaPlayer::durationChanged, this, &QQuickMediaPlayer::onDurationChanged);
    connect(this, &QMediaPlayer::mediaStatusChanged, this,
            &QQuickMediaPlayer::onMediaStatusChanged);
}

void QQuickMediaPlayer::qmlSetSource(const QUrl &source)
{
    if (m_source == source)
        return;
    m_source = source;
    m_wasMediaLoaded = false;
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

void QQuickMediaPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status != QMediaPlayer::LoadedMedia || std::exchange(m_wasMediaLoaded, true))
        return;

    // run with QueuedConnection to make the user able to handle the media status change
    // by themselves, otherwise play() might change the status in the handler.
    auto tryAutoPlay = [this]() {
        if (m_autoPlay && mediaStatus() == QMediaPlayer::LoadedMedia)
            play();
    };

    if (m_autoPlay)
        QMetaObject::invokeMethod(this, tryAutoPlay, Qt::QueuedConnection);
}

/*!
    \since 6.7
    \qmlproperty bool QtMultimedia::MediaPlayer::autoPlay

    This property controls whether the media begins to play automatically after it gets loaded.
    Defaults to \c false.
*/

bool QQuickMediaPlayer::autoPlay() const
{
    return m_autoPlay;
}

void QQuickMediaPlayer::setAutoPlay(bool autoPlay)
{
    if (std::exchange(m_autoPlay, autoPlay) != autoPlay)
        emit autoPlayChanged(autoPlay);
}

QT_END_NAMESPACE

#include "moc_qquickmediaplayer_p.cpp"
