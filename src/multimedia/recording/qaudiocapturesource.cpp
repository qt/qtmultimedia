/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmediaobject_p.h"
#include <qaudiocapturesource.h>
#include "qaudioendpointselector.h"

QT_BEGIN_NAMESPACE

/*!
    \class QAudioCaptureSource
    \brief The QAudioCaptureSource class provides an interface to query and select an audio input endpoint.
    \inmodule QtMultimedia
    \ingroup multimedia

    QAudioCaptureSource provides access to the audio inputs available on your system.

    You can query these inputs and select one to use.

    A typical implementation example:
    \snippet doc/src/snippets/multimedia-snippets/media.cpp Audio capture source

    The audiocapturesource interface is then used to:

    - Get and Set the audio input to use.

    The capture interface is then used to:

    - Set the destination using setOutputLocation()

    - Set the format parameters using setAudioCodec(),

    - Control the recording using record(),stop()

    \sa QMediaRecorder
*/

class QAudioCaptureSourcePrivate : public QMediaObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QAudioCaptureSource)

    void initControls()
    {
        Q_Q(QAudioCaptureSource);

        if (service != 0)
            audioEndpointSelector = qobject_cast<QAudioEndpointSelector*>(service->requestControl(QAudioEndpointSelector_iid));

        if (audioEndpointSelector) {
            q->connect(audioEndpointSelector, SIGNAL(activeEndpointChanged(const QString&)),
                       SIGNAL(activeAudioInputChanged(const QString&)));
            q->connect(audioEndpointSelector, SIGNAL(availableEndpointsChanged()),
                       SIGNAL(availableAudioInputsChanged()));
            q->connect(audioEndpointSelector, SIGNAL(availableEndpointsChanged()),
                       SLOT(statusChanged()));
            errorState = QtMultimedia::NoError;
        }
    }

    QAudioCaptureSourcePrivate():provider(0), audioEndpointSelector(0), errorState(QtMultimedia::ServiceMissingError) {}
    QMediaServiceProvider *provider;
    QAudioEndpointSelector   *audioEndpointSelector;
    QtMultimedia::AvailabilityError errorState;
};

/*!
    Construct a QAudioCaptureSource using the QMediaService from \a provider, with \a parent.
*/

QAudioCaptureSource::QAudioCaptureSource(QObject *parent, QMediaServiceProvider *provider):
    QMediaObject(*new QAudioCaptureSourcePrivate, parent, provider->requestService(Q_MEDIASERVICE_AUDIOSOURCE))
{
    Q_D(QAudioCaptureSource);

    d->provider = provider;
    d->initControls();
}

/*!
    Destroys the audiocapturesource object.
*/

QAudioCaptureSource::~QAudioCaptureSource()
{
    Q_D(QAudioCaptureSource);

    if (d->service && d->audioEndpointSelector)
        d->service->releaseControl(d->audioEndpointSelector);

    if (d->provider)
        d->provider->releaseService(d->service);
}

/*!
    Returns the error state of the audio capture service.
*/

QtMultimedia::AvailabilityError QAudioCaptureSource::availabilityError() const
{
    Q_D(const QAudioCaptureSource);

    return d->errorState;
}

/*!
    Returns true if the audio capture service is available, otherwise returns false.
*/
bool QAudioCaptureSource::isAvailable() const
{
    Q_D(const QAudioCaptureSource);

    if (d->service != NULL) {
        if (d->audioEndpointSelector && d->audioEndpointSelector->availableEndpoints().size() > 0)
            return true;
    }
    return false;
}


/*!
    Returns a list of available audio inputs
*/

QList<QString> QAudioCaptureSource::audioInputs() const
{
    Q_D(const QAudioCaptureSource);

    QList<QString> list;
    if (d && d->audioEndpointSelector)
        list <<d->audioEndpointSelector->availableEndpoints();

    return list;
}

/*!
    Returns the description of the audio input device with \a name.
*/

QString QAudioCaptureSource::audioDescription(const QString& name) const
{
    Q_D(const QAudioCaptureSource);

    if(d->audioEndpointSelector)
        return d->audioEndpointSelector->endpointDescription(name);
    else
        return QString();
}

/*!
    Returns the default audio input name.
*/

QString QAudioCaptureSource::defaultAudioInput() const
{
    Q_D(const QAudioCaptureSource);

    if(d->audioEndpointSelector)
        return d->audioEndpointSelector->defaultEndpoint();
    else
        return QString();
}

/*!
    Returns the active audio input name.
*/

QString QAudioCaptureSource::activeAudioInput() const
{
    Q_D(const QAudioCaptureSource);

    if(d->audioEndpointSelector)
        return d->audioEndpointSelector->activeEndpoint();
    else
        return QString();
}

/*!
    Set the active audio input to \a name.
*/

void QAudioCaptureSource::setAudioInput(const QString& name)
{
    Q_D(const QAudioCaptureSource);

    if(d->audioEndpointSelector)
        return d->audioEndpointSelector->setActiveEndpoint(name);
}

/*!
    \fn QAudioCaptureSource::activeAudioInputChanged(const QString& name)

    Signal emitted when active audio input changes to \a name.
*/

/*!
    \fn QAudioCaptureSource::availableAudioInputsChanged()

    Signal is emitted when the available audio inputs change.
*/

/*!
  \internal
*/
void QAudioCaptureSource::statusChanged()
{
    Q_D(QAudioCaptureSource);

    if (d->audioEndpointSelector) {
        if (d->audioEndpointSelector->availableEndpoints().size() > 0) {
            d->errorState = QtMultimedia::NoError;
            emit availabilityChanged(true);
        } else {
            d->errorState = QtMultimedia::BusyError;
            emit availabilityChanged(false);
        }
    } else {
        d->errorState = QtMultimedia::ServiceMissingError;
        emit availabilityChanged(false);
    }
}

#include "moc_qaudiocapturesource.cpp"
QT_END_NAMESPACE

