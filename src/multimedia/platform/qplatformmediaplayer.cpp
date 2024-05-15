// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformmediaplayer_p.h"
#include <private/qmediaplayer_p.h>
#include "qmediaplayer.h"
#include "qplatformmediadevices_p.h"
#include "qplatformmediaintegration_p.h"

QT_BEGIN_NAMESPACE

QPlatformMediaPlayer::QPlatformMediaPlayer(QMediaPlayer *parent) : player(parent)
{
    QPlatformMediaIntegration::instance()->mediaDevices()->prepareAudio();
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

void QPlatformMediaPlayer::error(int error, const QString &errorString)
{
    player->d_func()->setError(QMediaPlayer::Error(error), errorString);
}

QT_END_NAMESPACE
