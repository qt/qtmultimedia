// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidaudioinput_p.h"

#include <qaudioinput.h>

#include <QtCore/qjniobject.h>

QT_BEGIN_NAMESPACE

QAndroidAudioInput::QAndroidAudioInput(QAudioInput *parent)
    : QObject(parent),
      QPlatformAudioInput(parent)
{
    m_muted = isMuted();
}

QAndroidAudioInput::~QAndroidAudioInput()
{
    setMuted(m_muted);
}

void QAndroidAudioInput::setMuted(bool muted)
{
    bool isInputMuted = isMuted();
    if (muted != isInputMuted) {
        QJniObject::callStaticMethod<void>(
                    "org/qtproject/qt/android/multimedia/QtAudioDeviceManager",
                    "setInputMuted",
                    "(Z)V",
                    muted);
        emit mutedChanged(muted);
    }
}

bool QAndroidAudioInput::isMuted() const
{
    return QJniObject::callStaticMethod<jboolean>(
                   "org/qtproject/qt/android/multimedia/QtAudioDeviceManager",
                   "isMicrophoneMute",
                   "()Z");
}

QT_END_NAMESPACE

#include "moc_qandroidaudioinput_p.cpp"
