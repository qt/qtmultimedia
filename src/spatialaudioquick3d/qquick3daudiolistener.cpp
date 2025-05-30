// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#include <qquick3daudiolistener_p.h>
#include <qquick3dspatialsound_p.h>
#include <qquick3daudioengine_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype AudioListener
    \inqmlmodule QtQuick3D.SpatialAudio
    \ingroup quick3d_spatialaudio
    \ingroup multimedia_audio_qml

    \brief defines the position and orientation of the person listening to a sound field
    defined by a AudioEngine.

    A AudioEngine can have exactly one listener, that defines the position and orientation
    of the person listening to the sounds defined by the objects placed within the audio engine.

    In most cases, the AudioListener should simply be a child of the Camera element in QtQuick3D.
    This will ensure that the sound experience is aligned with the visual rendering of the scene.
 */

QQuick3DAudioListener::QQuick3DAudioListener()
{
    m_listener = new QAudioListener(QQuick3DAudioEngine::getEngine());
    connect(this, &QQuick3DNode::scenePositionChanged, this, &QQuick3DAudioListener::updatePosition);
    connect(this, &QQuick3DNode::sceneRotationChanged, this, &QQuick3DAudioListener::updateRotation);
    updatePosition();
    updateRotation();
}

QQuick3DAudioListener::~QQuick3DAudioListener()
{
    delete m_listener;
}

void QQuick3DAudioListener::updatePosition()
{
    m_listener->setPosition(scenePosition());
}

void QQuick3DAudioListener::updateRotation()
{
    m_listener->setRotation(sceneRotation());
}

QT_END_NAMESPACE
