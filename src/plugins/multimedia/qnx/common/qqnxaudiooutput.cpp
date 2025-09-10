// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxaudiooutput_p.h"

#include <private/qqnxaudiodevice_p.h>

#include <qaudiodevice.h>
#include <qaudiooutput.h>

#include <QtCore/qloggingcategory.h>

static Q_LOGGING_CATEGORY(qLcMediaAudioOutput, "qt.multimedia.audiooutput")

QT_BEGIN_NAMESPACE

QQnxAudioOutput::QQnxAudioOutput(QAudioOutput *parent)
  : QPlatformAudioOutput(parent)
{
}

QQnxAudioOutput::~QQnxAudioOutput()
{
}

void QQnxAudioOutput::setVolume(float vol)
{
    if (vol == volume)
        return;
    vol = volume;
    q->volumeChanged(vol);
}

void QQnxAudioOutput::setMuted(bool m)
{
    if (muted == m)
        return;
    muted = m;
    q->mutedChanged(muted);
}

void QQnxAudioOutput::setAudioDevice(const QAudioDevice &info)
{
    if (info == device)
        return;
    qCDebug(qLcMediaAudioOutput) << "setAudioDevice" << info.description() << info.isNull();
    device = info;

    // ### handle device changes
}

QT_END_NAMESPACE
