// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#include "qaudiolistener.h"

#include <QtSpatialAudio/private/qaudioengine_p.h>
#include <QtMultimedia/qaudiosink.h>
#include <QtCore/private/qobject_p.h>


QT_BEGIN_NAMESPACE

class QAudioListenerPrivate : public QObjectPrivate
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

    \note Must be called with a valid QAudioEngine
 */
QAudioListener::QAudioListener(QAudioEngine *engine) : QObject(*new QAudioListenerPrivate)
{
    Q_D(QAudioListener);

    d->engine = engine;
    if (d->engine) {
        auto *ed = QAudioEnginePrivate::get(d->engine);
        bool hasListener = ed->listenerPosition().has_value();
        if (hasListener) {
            qWarning() << "Ignoring attempt to add a second listener to the spatial audio engine.";
            d->engine = nullptr;
        } else {
            ed->setListenerPosition(d->pos);
        }
    }
}

/*!
    Destroys the listener.
 */
QAudioListener::~QAudioListener()
{
    Q_D(QAudioListener);

    if (d->engine) {
        auto *ed = QAudioEnginePrivate::get(d->engine);
        ed->setListenerPosition(std::nullopt);
    }
    d->engine = nullptr;
}

/*!
    Sets the listener's position in 3D space to \a pos. Units are in centimeters
    by default.

    \sa QAudioEngine::distanceScale
 */
void QAudioListener::setPosition(QVector3D pos)
{
    Q_D(QAudioListener);

    d->pos = pos;
    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (!ep)
        return;
    ep->setListenerPosition(pos);
}

/*!
    Returns the current position of the listener.
 */
QVector3D QAudioListener::position() const
{
    Q_D(const QAudioListener);
    return d->pos;
}

/*!
    Sets the listener's orientation in 3D space to \a q.
 */
void QAudioListener::setRotation(const QQuaternion &q)
{
    Q_D(QAudioListener);
    d->rotation = q;
    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->setListenerRotation(q);
}

/*!
    Returns the listener's orientation in 3D space.
 */
QQuaternion QAudioListener::rotation() const
{
    Q_D(const QAudioListener);
    return d->rotation;
}

/*!
    Returns the engine associated with this listener.
 */
QAudioEngine *QAudioListener::engine() const
{
    Q_D(const QAudioListener);
    return d->engine;
}

QT_END_NAMESPACE
