// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformmediaplayer_p.h"
#include <private/qmediaplayer_p.h>
#include "qmediaplayer.h"

QT_BEGIN_NAMESPACE

QPlatformMediaPlayer::~QPlatformMediaPlayer()
{
}

void QPlatformMediaPlayer::stateChanged(QMediaPlayer::PlaybackState newState)
{
    if (newState == m_state)
        return;
    m_state = newState;
    player->d_func()->setState(newState);
}

void QPlatformMediaPlayer::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (m_status == status)
        return;
    m_status = status;
    player->d_func()->setStatus(status);
}

void QPlatformMediaPlayer::error(int error, const QString &errorString)
{
    player->d_func()->setError(error, errorString);
}

QT_END_NAMESPACE
