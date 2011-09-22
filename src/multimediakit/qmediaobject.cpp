/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qmetaobject.h>
#include <QtCore/qdebug.h>

#include "qmediaobject_p.h"

#include <qmediaservice.h>
#include <qmetadatareadercontrol.h>
#include <qmediabindableinterface.h>


QT_BEGIN_NAMESPACE

void QMediaObjectPrivate::_q_notify()
{
    Q_Q(QMediaObject);

    const QMetaObject* m = q->metaObject();

    foreach (int pi, notifyProperties) {
        QMetaProperty p = m->property(pi);
        p.notifySignal().invoke(
            q, QGenericArgument(QMetaType::typeName(p.userType()), p.read(q).data()));
    }
}


/*!
    \class QMediaObject

    \brief The QMediaObject class provides a common base for multimedia objects.

    \inmodule QtMultimediaKit
    \ingroup multimedia
    \since 1.0

    QMediaObject derived classes provide access to the functionality of a
    QMediaService.  Each media object hosts a QMediaService and uses the
    QMediaControl interfaces implemented by the service to implement its
    API.  Most media objects when constructed will request a new
    QMediaService instance from a QMediaServiceProvider, but some like
    QMediaRecorder will share a service with another object.

    QMediaObject itself provides an API for accessing a media
    service's \l {metaData()}{meta-data} and a means of connecting other media objects,
    and peripheral classes like QVideoWidget and QMediaPlaylist.

    \sa QMediaService, QMediaControl
*/

/*!
    Destroys this media object.
*/

QMediaObject::~QMediaObject()
{
    delete d_ptr;
}

/*!
    Returns the service availability error state.
    \since 1.0
*/

QtMultimediaKit::AvailabilityError QMediaObject::availabilityError() const
{
    return d_func()->service == 0 ? QtMultimediaKit::ServiceMissingError : QtMultimediaKit::NoError;
}

/*!
    Returns true if the service is available for use.
    \since 1.0
*/

bool QMediaObject::isAvailable() const
{
    return d_func()->service != 0;
}

/*!
    Returns the media service that provides the functionality of this multimedia object.
    \since 1.0
*/

QMediaService* QMediaObject::service() const
{
    return d_func()->service;
}

int QMediaObject::notifyInterval() const
{
    return d_func()->notifyTimer->interval();
}

void QMediaObject::setNotifyInterval(int milliSeconds)
{
    Q_D(QMediaObject);

    if (d->notifyTimer->interval() != milliSeconds) {
        d->notifyTimer->setInterval(milliSeconds);

        emit notifyIntervalChanged(milliSeconds);
    }
}

/*!
    Bind \a object to this QMediaObject instance.

    This method establishes a relationship between this media object and a
    helper object. The nature of the relationship depends on both parties. This
    methods returns true if the helper was successfully bound, false otherwise.

    Most subclasses of QMediaObject provide more convenient functions
    that wrap this functionality, so this function rarely needs to be
    called directly.

    The object passed must implement the QMediaBindableInterface interface.

    \since 1.0
    \sa QMediaBindableInterface
*/
bool QMediaObject::bind(QObject *object)
{
    QMediaBindableInterface *helper = qobject_cast<QMediaBindableInterface*>(object);
    if (!helper)
        return false;

    QMediaObject *currentObject = helper->mediaObject();

    if (currentObject == this)
        return true;

    if (currentObject)
        currentObject->unbind(object);

    return helper->setMediaObject(this);
}

/*!
    Detach \a object from the QMediaObject instance.

    Unbind the helper object from this media object.  A warning
    will be generated if the object was not previously bound to this
    object.

    \since 1.0
    \sa QMediaBindableInterface
*/
void QMediaObject::unbind(QObject *object)
{
    QMediaBindableInterface *helper = qobject_cast<QMediaBindableInterface*>(object);

    if (helper && helper->mediaObject() == this)
        helper->setMediaObject(0);
    else
        qWarning() << "QMediaObject: Trying to unbind not connected helper object";
}

/*!
    Constructs a media object which uses the functionality provided by a media \a service.

    The \a parent is passed to QObject.

    This class is meant as a base class for multimedia objects so this
    constructor is protected.
    \since 1.0
*/

QMediaObject::QMediaObject(QObject *parent, QMediaService *service):
    QObject(parent),
    d_ptr(new QMediaObjectPrivate)

{
    Q_D(QMediaObject);

    d->q_ptr = this;

    d->notifyTimer = new QTimer(this);
    d->notifyTimer->setInterval(1000);
    connect(d->notifyTimer, SIGNAL(timeout()), SLOT(_q_notify()));

    d->service = service;

    setupMetaData();
}

/*!
    \internal
*/

QMediaObject::QMediaObject(QMediaObjectPrivate &dd, QObject *parent,
                                            QMediaService *service):
    QObject(parent),
    d_ptr(&dd)
{
    Q_D(QMediaObject);
    d->q_ptr = this;

    d->notifyTimer = new QTimer(this);
    d->notifyTimer->setInterval(1000);
    connect(d->notifyTimer, SIGNAL(timeout()), SLOT(_q_notify()));

    d->service = service;

    setupMetaData();
}

/*!
    Watch the property \a name. The property's notify signal will be emitted
    once every \code notifyInterval milliseconds.

    \since 1.0
    \sa notifyInterval
*/

void QMediaObject::addPropertyWatch(QByteArray const &name)
{
    Q_D(QMediaObject);

    const QMetaObject* m = metaObject();

    int index = m->indexOfProperty(name.constData());

    if (index != -1 && m->property(index).hasNotifySignal()) {
        d->notifyProperties.insert(index);

        if (!d->notifyTimer->isActive())
            d->notifyTimer->start();
    }
}

/*!
    Remove property \a name from the list of properties whose changes are
    regularly signaled.

    \since 1.0
    \sa notifyInterval
*/

void QMediaObject::removePropertyWatch(QByteArray const &name)
{
    Q_D(QMediaObject);

    int index = metaObject()->indexOfProperty(name.constData());

    if (index != -1) {
        d->notifyProperties.remove(index);

        if (d->notifyProperties.isEmpty())
            d->notifyTimer->stop();
    }
}

/*!
    \property QMediaObject::notifyInterval

    The interval at which notifiable properties will update.

    The interval is expressed in milliseconds, the default value is 1000.

    \since 1.0
    \sa addPropertyWatch(), removePropertyWatch()
*/

/*!
    \fn void QMediaObject::notifyIntervalChanged(int milliseconds)

    Signal a change in the notify interval period to \a milliseconds.
    \since 1.0
*/

/*!
    Returns true if there is meta-data associated with this media object, else false.
    \since 1.0
*/

bool QMediaObject::isMetaDataAvailable() const
{
    Q_D(const QMediaObject);

    return d->metaDataControl
            ? d->metaDataControl->isMetaDataAvailable()
            : false;
}

/*!
    \fn QMediaObject::metaDataAvailableChanged(bool available)

    Signals that the \a available state of a media object's meta-data has changed.
    \since 1.0
*/

/*!
    Returns the value associated with a meta-data \a key.
    \since 1.0
*/
QVariant QMediaObject::metaData(QtMultimediaKit::MetaData key) const
{
    Q_D(const QMediaObject);

    return d->metaDataControl
            ? d->metaDataControl->metaData(key)
            : QVariant();
}

/*!
    Returns a list of keys there is meta-data available for.
    \since 1.0
*/
QList<QtMultimediaKit::MetaData> QMediaObject::availableMetaData() const
{
    Q_D(const QMediaObject);

    return d->metaDataControl
            ? d->metaDataControl->availableMetaData()
            : QList<QtMultimediaKit::MetaData>();
}

/*!
    \fn QMediaObject::metaDataChanged()

    Signals that this media object's meta-data has changed.
    \since 1.0
*/

/*!
    Returns the value associated with a meta-data \a key.

    The naming and type of extended meta-data is not standardized, so the values and meaning
    of keys may vary between backends.
    \since 1.0
*/
QVariant QMediaObject::extendedMetaData(const QString &key) const
{
    Q_D(const QMediaObject);

    return d->metaDataControl
            ? d->metaDataControl->extendedMetaData(key)
            : QVariant();
}

/*!
    Returns a list of keys there is extended meta-data available for.
    \since 1.0
*/
QStringList QMediaObject::availableExtendedMetaData() const
{
    Q_D(const QMediaObject);

    return d->metaDataControl
            ? d->metaDataControl->availableExtendedMetaData()
            : QStringList();
}


void QMediaObject::setupMetaData()
{
    Q_D(QMediaObject);

    if (d->service != 0) {
        d->metaDataControl = qobject_cast<QMetaDataReaderControl*>(
                d->service->requestControl(QMetaDataReaderControl_iid));

        if (d->metaDataControl) {
            connect(d->metaDataControl, SIGNAL(metaDataChanged()), SIGNAL(metaDataChanged()));
            connect(d->metaDataControl,
                    SIGNAL(metaDataAvailableChanged(bool)),
                    SIGNAL(metaDataAvailableChanged(bool)));
        }
    }
}

/*!
    \fn QMediaObject::availabilityChanged(bool available)

    Signal emitted when the availability state has changed to \a available
    \since 1.0
*/


#include "moc_qmediaobject.cpp"
QT_END_NAMESPACE

