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
#include "qspatialaudiolistener.h"
#include "qspatialaudioengine_p.h"
#include "api/resonance_audio_api.h"
#include <qaudiosink.h>
#include <qurl.h>
#include <qdebug.h>
#include <qaudiodecoder.h>

QT_BEGIN_NAMESPACE

class QSpatialAudioListenerPrivate
{
public:
    QSpatialAudioEngine *engine = nullptr;
    QVector3D pos;
    QQuaternion rotation;
};

/*!
    \class QSpatialAudioListener
    \inmodule QtMultimedia
    \ingroup multimedia_spatialaudio

    \brief Defines the position and orientation of the person listening to a sound field
    defined by QSpatialAudioEngine.

    A QSpatialAudioEngine can have exactly one listener that defines the position and orientation
    of the person listening to the sound field.
 */

/*!
    Creates a listener for the spatial audio engine for \a engine.
 */
QSpatialAudioListener::QSpatialAudioListener(QSpatialAudioEngine *engine)
    : d(new QSpatialAudioListenerPrivate)
{
    setEngine(engine);
}

/*!
    Destroys the listener.
 */
QSpatialAudioListener::~QSpatialAudioListener()
{
    delete d;
}

/*!
    Sets the listener's position in 3D space to \a pos. Units are assumed to
    represent meters.
 */
void QSpatialAudioListener::setPosition(QVector3D pos)
{
    if (d->pos == pos)
        return;

    d->pos = pos;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep && ep->api) {
        ep->api->SetHeadPosition(pos.x(), pos.y(), pos.z());
        ep->listenerPositionDirty = true;
    }
}

/*!
    Returns the current position of the listener.
 */
QVector3D QSpatialAudioListener::position() const
{
    return d->pos;
}

/*!
    Sets the listener's orientation in 3D space to \a q.
 */
void QSpatialAudioListener::setRotation(const QQuaternion &q)
{
    d->rotation = q;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep && ep->api)
        ep->api->SetHeadRotation(d->rotation.x(), d->rotation.y(), d->rotation.z(), d->rotation.scalar());
}

/*!
    Returns the listener's orientation in 3D space.
 */
QQuaternion QSpatialAudioListener::rotation() const
{
    return d->rotation;
}

/*!
    \internal
 */
void QSpatialAudioListener::setEngine(QSpatialAudioEngine *engine)
{
    if (d->engine) {
        auto *ed = QSpatialAudioEnginePrivate::get(d->engine);
        ed->listener = nullptr;
    }
    d->engine = engine;
    if (d->engine) {
        auto *ed = QSpatialAudioEnginePrivate::get(d->engine);
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
QSpatialAudioEngine *QSpatialAudioListener::engine() const
{
    return d->engine;
}

QT_END_NAMESPACE
