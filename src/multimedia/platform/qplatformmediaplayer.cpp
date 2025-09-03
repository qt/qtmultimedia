// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformmediaplayer_p.h"
#include <private/qmediaplayer_p.h>
#include "qmediaplayer.h"
#include "qplatformaudiodevices_p.h"
#include "qplatformmediaintegration_p.h"

QT_BEGIN_NAMESPACE

QPlatformMediaPlayer::QPlatformMediaPlayer(QMediaPlayer *parent) : player(parent)
{
}

QPlatformMediaPlayer::~QPlatformMediaPlayer() = default;

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

void QPlatformMediaPlayer::error(QMediaPlayer::Error error, const QString &errorString)
{
    player->d_func()->setError(error, errorString);
}

QPlatformMediaPlayer::PitchCompensationAvailability
QPlatformMediaPlayer::pitchCompensationAvailability() const
{
    return PitchCompensationAvailability::Unavailable;
}

void QPlatformMediaPlayer::setPitchCompensation(bool /*enabled*/)
{
    qWarning() << "QMediaPlayer::setPitchCompensation not supported on this QtMultimedia "
                  "backend";
}

bool QPlatformMediaPlayer::pitchCompensation() const
{
    return false;
}

void QPlatformMediaPlayer::pitchCompensationChanged(bool enabled) const
{
    emit player->pitchCompensationChanged(enabled);
}

QPlaybackOptions QPlatformMediaPlayer::playbackOptions() const
{
    return player->playbackOptions();
}

QT_END_NAMESPACE
