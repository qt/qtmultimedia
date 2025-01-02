// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#include "qaudiolistener.h"
#include "qaudioengine_p.h"
#include "resonance_audio.h"
#include <qaudiosink.h>
#include <qurl.h>
#include <qdebug.h>
#include <qaudiodecoder.h>

QT_BEGIN_NAMESPACE

class QAudioListenerPrivate
{
public:
    QAudioEngine *engine = nullptr;
    QVector3D pos;
    QQuaternion rotation;
};

/*!
    \class QAudioListener
    \inmodule QtSpatialAudio
    \ingroup spatialaudio
    \ingroup multimedia_audio

    \brief Defines the position and orientation of the person listening to a sound field
    defined by QAudioEngine.

    A QAudioEngine can have exactly one listener that defines the position and orientation
    of the person listening to the sound field.
 */

/*!
    Creates a listener for the spatial audio engine for \a engine.
 */
QAudioListener::QAudioListener(QAudioEngine *engine)
    : d(new QAudioListenerPrivate)
{
    setEngine(engine);
}

/*!
    Destroys the listener.
 */
QAudioListener::~QAudioListener()
{
    delete d;
}

/*!
    Sets the listener's position in 3D space to \a pos. Units are in centimeters
    by default.

    \sa QAudioEngine::distanceScale
 */
void QAudioListener::setPosition(QVector3D pos)
{
    auto *ep = QAudioEnginePrivate::get(d->engine);
    pos *= ep->distanceScale;
    if (d->pos == pos)
        return;

    d->pos = pos;
    if (ep && ep->resonanceAudio->api) {
        ep->resonanceAudio->api->SetHeadPosition(pos.x(), pos.y(), pos.z());
        ep->listenerPositionDirty = true;
    }
}

/*!
    Returns the current position of the listener.
 */
QVector3D QAudioListener::position() const
{
    auto *ep = QAudioEnginePrivate::get(d->engine);
    return d->pos/ep->distanceScale;
}

/*!
    Sets the listener's orientation in 3D space to \a q.
 */
void QAudioListener::setRotation(const QQuaternion &q)
{
    d->rotation = q;
    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (ep && ep->resonanceAudio->api)
        ep->resonanceAudio->api->SetHeadRotation(d->rotation.x(), d->rotation.y(), d->rotation.z(), d->rotation.scalar());
}

/*!
    Returns the listener's orientation in 3D space.
 */
QQuaternion QAudioListener::rotation() const
{
    return d->rotation;
}

/*!
    \internal
 */
void QAudioListener::setEngine(QAudioEngine *engine)
{
    if (d->engine) {
        auto *ed = QAudioEnginePrivate::get(d->engine);
        ed->listener = nullptr;
    }
    d->engine = engine;
    if (d->engine) {
        auto *ed = QAudioEnginePrivate::get(d->engine);
        if (ed->listener) {
            qWarning() << "Ignoring attempt to add a second listener to the spatial audio engine.";
            d->engine = nullptr;
            return;
        }
        ed->listener = this;
    }
}

/*!
    Returns the engine associated with this listener.
 */
QAudioEngine *QAudioListener::engine() const
{
    return d->engine;
}

QT_END_NAMESPACE
