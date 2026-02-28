// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmediaplayer_p.h"

#include <QtMultimedia/private/qmediaplayer_p.h>
#include <QtMultimediaQuick/private/qqmlcontext_source_resolver_p.h>

QT_BEGIN_NAMESPACE

QQuickMediaPlayer::QQuickMediaPlayer(QObject *parent) : QMediaPlayer(parent)
{
    connect(this, &QMediaPlayer::mediaStatusChanged, this,
            &QQuickMediaPlayer::onMediaStatusChanged);

    auto *playerPrivate = QMediaPlayerPrivate::get(this);
    playerPrivate->m_sourceResolver =
            std::make_unique<QMultimediaPrivate::QQmlContextSourceResolver>(this);
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
