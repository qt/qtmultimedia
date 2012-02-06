/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qaudiorecorder.h"
#include "qaudioendpointselector.h"
#include "qmediaobject_p.h"
#include "qmediarecorder_p.h"
#include <qmediaservice.h>
#include <qmediaserviceprovider_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qurl.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qmetaobject.h>

#include <qaudioformat.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAudioRecorder
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_recording

    \brief The QAudioRecorder class is used for the recording of media content.

    The QAudioRecorder class is a high level media recording class.
*/

class QAudioRecorderObject : public QMediaObject
{
public:
    QAudioRecorderObject(QObject *parent, QMediaService *service)
        :QMediaObject(parent, service)
    {
    }

    ~QAudioRecorderObject()
    {
    }
};

class QAudioRecorderPrivate : public QMediaRecorderPrivate
{
    Q_DECLARE_NON_CONST_PUBLIC(QAudioRecorder)

public:
    void initControls()
    {
        Q_Q(QAudioRecorder);
        audioEndpointSelector = 0;

        QMediaService *service = mediaObject ? mediaObject->service() : 0;

        if (service != 0)
            audioEndpointSelector = qobject_cast<QAudioEndpointSelector*>(service->requestControl(QAudioEndpointSelector_iid));

        if (audioEndpointSelector) {
            q->connect(audioEndpointSelector, SIGNAL(activeEndpointChanged(QString)),
                       SIGNAL(audioInputChanged(QString)));
            q->connect(audioEndpointSelector, SIGNAL(availableEndpointsChanged()),
                       SIGNAL(availableAudioInputsChanged()));
        }
    }

    QAudioRecorderPrivate():
        QMediaRecorderPrivate(),
        provider(0),
        audioEndpointSelector(0) {}

    QMediaServiceProvider *provider;
    QAudioEndpointSelector   *audioEndpointSelector;
};



/*!
    Constructs an audio recorder.
    The \a parent is passed to QMediaObject.
*/

QAudioRecorder::QAudioRecorder(QObject *parent):
    QMediaRecorder(*new QAudioRecorderPrivate, 0, parent)
{
    Q_D(QAudioRecorder);
    d->provider = QMediaServiceProvider::defaultServiceProvider();

    QMediaService *service = d->provider->requestService(Q_MEDIASERVICE_AUDIOSOURCE);
    setMediaObject(new QAudioRecorderObject(this, service));
    d->initControls();
}

/*!
    Destroys an audio recorder object.
*/

QAudioRecorder::~QAudioRecorder()
{
    Q_D(QAudioRecorder);
    QMediaService *service = d->mediaObject ? d->mediaObject->service() : 0;
    QMediaObject *mediaObject = d->mediaObject;
    setMediaObject(0);

    if (service && d->audioEndpointSelector)
        service->releaseControl(d->audioEndpointSelector);

    if (d->provider && service)
        d->provider->releaseService(service);

    delete mediaObject;
}

/*!
    Returns a list of available audio inputs
*/

QStringList QAudioRecorder::audioInputs() const
{
    Q_D(const QAudioRecorder);
    if (d->audioEndpointSelector)
        return d->audioEndpointSelector->availableEndpoints();
    else
        return QStringList();
}

/*!
    Returns the readable translated description of the audio input device with \a name.
*/

QString QAudioRecorder::audioInputDescription(const QString& name) const
{
    Q_D(const QAudioRecorder);

    if (d->audioEndpointSelector)
        return d->audioEndpointSelector->endpointDescription(name);
    else
        return QString();
}

/*!
    Returns the default audio input name.
*/

QString QAudioRecorder::defaultAudioInput() const
{
    Q_D(const QAudioRecorder);

    if (d->audioEndpointSelector)
        return d->audioEndpointSelector->defaultEndpoint();
    else
        return QString();
}

/*!
    Returns the active audio input name.
*/

QString QAudioRecorder::audioInput() const
{
    Q_D(const QAudioRecorder);

    if (d->audioEndpointSelector)
        return d->audioEndpointSelector->activeEndpoint();
    else
        return QString();
}

/*!
    Set the active audio input to \a name.
*/

void QAudioRecorder::setAudioInput(const QString& name)
{
    Q_D(const QAudioRecorder);

    if (d->audioEndpointSelector)
        return d->audioEndpointSelector->setActiveEndpoint(name);
}

/*!
    \fn QAudioRecorder::activeAudioInputChanged(const QString& name)

    Signal emitted when active audio input changes to \a name.
*/

/*!
    \fn QAudioRecorder::availableAudioInputsChanged()

    Signal is emitted when the available audio inputs change.
*/



#include "moc_qaudiorecorder.cpp"
QT_END_NAMESPACE

