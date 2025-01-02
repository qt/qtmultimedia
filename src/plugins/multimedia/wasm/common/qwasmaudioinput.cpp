// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR
// GPL-2.0-only OR GPL-3.0-only

#include "qwasmaudioinput_p.h"

#include <qaudioinput.h>

QT_BEGIN_NAMESPACE

QWasmAudioInput::QWasmAudioInput(QAudioInput *parent)
    : QObject(parent),
      QPlatformAudioInput(parent)
{
    m_wasMuted = false;
}

QWasmAudioInput::~QWasmAudioInput() { }

void QWasmAudioInput::setMuted(bool muted)
{
    if (muted != m_wasMuted)
        emit mutedChanged(muted);
    m_wasMuted = muted;
}

bool QWasmAudioInput::isMuted() const
{
    return m_wasMuted;
}

QT_END_NAMESPACE

#include "moc_qwasmaudioinput_p.cpp"
