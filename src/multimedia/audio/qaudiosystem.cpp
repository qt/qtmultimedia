// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaudiosystem_p.h"

#include <private/qplatformmediadevices_p.h>

QT_BEGIN_NAMESPACE

QAudioStateChangeNotifier::QAudioStateChangeNotifier(QObject *parent) : QObject(parent) { }

QPlatformAudioSink::QPlatformAudioSink(QObject *parent) : QAudioStateChangeNotifier(parent) { }

qreal QPlatformAudioSink::volume() const
{
    return 1.0;
}

QPlatformAudioSource::QPlatformAudioSource(QObject *parent) : QAudioStateChangeNotifier(parent) { }

QT_END_NAMESPACE

#include "moc_qaudiosystem_p.cpp"
