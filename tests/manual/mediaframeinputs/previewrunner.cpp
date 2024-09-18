// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "previewrunner.h"

#include <QEvent>

PreviewRunner::PreviewRunner()
{
    m_player.setVideoOutput(&m_preview);
    m_player.setAudioOutput(&m_audioOutput);

    connect(&m_player, &QMediaPlayer::playbackStateChanged, this,
            &PreviewRunner::handlePlaybackStateChanged);
    connect(&m_player, &QMediaPlayer::errorOccurred, this, &PreviewRunner::handleError);
    connect(&m_player, &QMediaPlayer::positionChanged, this, &PreviewRunner::updateTitle);

    m_preview.installEventFilter(this);
}

bool PreviewRunner::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == &m_preview && event->type() == QEvent::Close) {
        emit finished();
        m_player.stop();
    }

    return QObject::eventFilter(watched, event);
}

void PreviewRunner::run(const QUrl &mediaLocation)
{
    m_preview.show();
    m_player.setSource(mediaLocation);
    m_player.play();
}

void PreviewRunner::handlePlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::StoppedState) {
        m_preview.close();

        const bool noError = m_player.error() == QMediaPlayer::NoError;
        qInfo() << "Preview finished" << (noError ? "" : "with an error");
    } else if (state == QMediaPlayer::PlayingState) {
        qInfo() << "Running preview for media" << m_player.source();
        updateTitle();
    }
}

void PreviewRunner::handleError(QMediaPlayer::Error error, QString description)
{
    qWarning() << "Playing error occurred:" << error << description;

    if (m_player.playbackState() == QMediaPlayer::StoppedState)
        m_preview.close();
}

void PreviewRunner::updateTitle()
{
    m_preview.setWindowTitle(QStringLiteral("Playing %1; pos: %2; duration: %3")
                                     .arg(m_player.source().toString(),
                                          QString::number(m_player.position()),
                                          QString::number(m_player.duration())));
}

#include "moc_previewrunner.cpp"
