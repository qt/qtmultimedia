// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxaudioinput_p.h"

QT_BEGIN_NAMESPACE

QQnxAudioInput::QQnxAudioInput(QAudioInput *parent)
    : QPlatformAudioInput(parent)
{
}

QQnxAudioInput::~QQnxAudioInput()
{
}

void QQnxAudioInput::setAudioDevice(const QAudioDevice &info)
{
    if (info == device)
        return;

    device = info;
}

QT_END_NAMESPACE
