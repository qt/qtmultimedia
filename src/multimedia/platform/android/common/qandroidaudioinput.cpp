/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
