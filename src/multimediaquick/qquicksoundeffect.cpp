// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicksoundeffect_p.h"

#include <QtMultimedia/private/qsoundeffect_p.h>
#include <QtMultimediaQuick/private/qqmlcontext_source_resolver_p.h>

QT_BEGIN_NAMESPACE

QQuickSoundEffect::QQuickSoundEffect(QObject *parent) : QSoundEffect(parent)
{
    auto *fxPrivate = QSoundEffectPrivate::get(this);
    fxPrivate->m_sourceResolver =
            std::make_unique<QMultimediaPrivate::QQmlContextSourceResolver>(this);
}

QT_END_NAMESPACE

#include "moc_qquicksoundeffect_p.cpp"
