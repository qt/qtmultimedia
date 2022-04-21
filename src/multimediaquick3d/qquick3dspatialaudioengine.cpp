/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Spatial Audio module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-NOGPL2$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qquick3dspatialaudioengine_p.h>

QT_BEGIN_NAMESPACE

static QSpatialAudioEngine *globalEngine = nullptr;

QQuick3DSpatialAudioEngine::QQuick3DSpatialAudioEngine()
{
    auto *e = getEngine();
    connect(e, &QSpatialAudioEngine::outputModeChanged, this, &QQuick3DSpatialAudioEngine::outputModeChanged);
    connect(e, &QSpatialAudioEngine::outputDeviceChanged, this, &QQuick3DSpatialAudioEngine::outputDeviceChanged);
    connect(e, &QSpatialAudioEngine::masterVolumeChanged, this, &QQuick3DSpatialAudioEngine::masterVolumeChanged);
}

QQuick3DSpatialAudioEngine::~QQuick3DSpatialAudioEngine()
{
}

void QQuick3DSpatialAudioEngine::setOutputMode(OutputMode mode)
{
    globalEngine->setOutputMode(QSpatialAudioEngine::OutputMode(mode));
}

QQuick3DSpatialAudioEngine::OutputMode QQuick3DSpatialAudioEngine::outputMode() const
{
    return OutputMode(globalEngine->outputMode());
}

void QQuick3DSpatialAudioEngine::setOutputDevice(const QAudioDevice &device)
{
    globalEngine->setOutputDevice(device);
}

QAudioDevice QQuick3DSpatialAudioEngine::outputDevice() const
{
    return globalEngine->outputDevice();
}

void QQuick3DSpatialAudioEngine::setMasterVolume(float volume)
{
    globalEngine->setMasterVolume(volume);
}

float QQuick3DSpatialAudioEngine::masterVolume() const
{
    return globalEngine->masterVolume();
}

QSpatialAudioEngine *QQuick3DSpatialAudioEngine::getEngine()
{
    if (!globalEngine) {
        globalEngine = new QSpatialAudioEngine;
        globalEngine->start();
    }
    return globalEngine;
}

QT_END_NAMESPACE

#include "moc_qquick3dspatialaudioengine_p.cpp"
