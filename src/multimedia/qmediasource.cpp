/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
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
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qmetaobject.h>
#include <QtCore/qdebug.h>

#include "qmediasource_p.h"

#include <qmediaservice.h>
#include <qmediasink.h>
#include <qmediametadata.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMediaSource

    \brief The QMediaSource class provides a common base for multimedia objects.
    \inmodule QtMultimedia

    \ingroup multimedia
    \ingroup multimedia_core

    It provides some basic functionality that is common to other high level classes
    like \l QMediaPlayer, \l QAudioDecoder and \l QCamera, including availability
    and meta-data functionality.

    The higher level QMediaSource derived classes provide the actual multimedia
    functionality, by internally using a QMediaService.  Each media object
    hosts a QMediaService and uses the QMediaControl interfaces implemented by the service to implement its
    API.  These controls can be accessed from the media object if necessary, but in general
    the useful functionality can be accessed from the higher level classes.

    Most media objects when constructed will request a new
    QMediaService instance, but some like
    QMediaRecorder and QMediaRecorder will share a service with another object.

    \sa QMediaService, QMediaControl
*/

/*!
    Destroys this media object.
*/

QMediaSource::~QMediaSource()
{
}

/*!
    Returns the availability of the functionality offered by this object.

    In some cases the functionality may not be available (for example, if
    the current operating system or platform does not provide the required
    functionality), or it may be temporarily unavailable (for example,
    audio playback during a phone call or similar).
*/

QMultimedia::AvailabilityStatus QMediaSource::availability() const
{
    if (d_func()->service == nullptr)
        return QMultimedia::ServiceMissing;

    return QMultimedia::Available;
}

/*!
    \fn bool QMediaSource::isAvailable() const

    Returns true if the service is available for use.
*/

/*!
    Returns the media service that provides the functionality of this multimedia object.
*/

QMediaService* QMediaSource::service() const
{
    return d_func()->service;
}

/*!
    Bind \a object to this QMediaSource instance.

    This method establishes a relationship between this media object and a
    helper object. The nature of the relationship depends on both parties. This
    methods returns true if the helper was successfully bound, false otherwise.

    Most subclasses of QMediaSource provide more convenient functions
    that wrap this functionality, so this function rarely needs to be
    called directly.

    The object passed must implement the QMediaSink interface.

    \sa QMediaSink
*/
bool QMediaSource::bind(QMediaSink *sink)
{
    if (!sink)
        return false;

    QMediaSource *currentObject = sink->mediaSource();

    if (currentObject == this)
        return true;

    if (currentObject)
        currentObject->unbind(sink);

    return sink->setMediaSource(this);
}

/*!
    Detach \a object from the QMediaSource instance.

    Unbind the helper object from this media object.  A warning
    will be generated if the object was not previously bound to this
    object.

    \sa QMediaSink
*/
void QMediaSource::unbind(QMediaSink *sink)
{
    if (sink && sink->mediaSource() == this)
        sink->setMediaSource(nullptr);
    else
        qWarning() << "QMediaSource: Trying to unbind not connected helper object";
}

/*!
    Constructs a media object which uses the functionality provided by a media \a service.

    The \a parent is passed to QObject.

    This class is meant as a base class for multimedia objects so this
    constructor is protected.
*/

QMediaSource::QMediaSource(QObject *parent, QMediaService *service)
    : QObject(*new QMediaSourcePrivate, parent)
{
    Q_D(QMediaSource);

    d->service = service;
}

/*!
    \internal
*/

QMediaSource::QMediaSource(QMediaSourcePrivate &dd, QObject *parent, QMediaService *service)
    : QObject(dd, parent)
{
    Q_D(QMediaSource);

    d->service = service;
}

/*!
    Returns the value associated with a meta-data \a key.

    See the list of predefined \l {QMediaMetaData}{meta-data keys}.
*/
QMediaMetaData QMediaSource::metaData() const
{
    return QMediaMetaData();
}

/*!
    \fn QMediaSource::metaDataChanged()

    Signals that this media object's meta-data has changed.

    If multiple meta-data elements are changed,
    metaDataChanged(const QString &key, const QVariant &value) signal is emitted
    for each of them with metaDataChanged() changed emitted once.
*/

QT_END_NAMESPACE

#include "moc_qmediasource.cpp"
