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

#include "qaudiorecorder.h"
#include "qaudioinputselectorcontrol.h"
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

    \brief The QAudioRecorder class is used for the recording of audio.

    The QAudioRecorder class is a high level media recording class and contains
    the same functionality as \l QMediaRecorder.

    \snippet multimedia-snippets/media.cpp Audio recorder

    In addition QAudioRecorder provides functionality for selecting the audio input.

    \snippet multimedia-snippets/media.cpp Audio recorder inputs

    The \l {Audio Recorder Example} shows how to use this class in more detail.

    \sa QMediaRecorder, QAudioInputSelectorControl
*/

class QAudioRecorderObject : public QMediaObject
{
public:
    QAudioRecorderObject(QObject *parent, QMediaService *service)
        : QMediaObject(parent, service)
    {
    }

    ~QAudioRecorderObject() override
    {
    }
};

class QAudioRecorderPrivate : public QMediaRecorderPrivate
{
public:
    QMediaServiceProvider *provider = nullptr;
};



/*!
    Constructs an audio recorder.
    The \a parent is passed to QMediaObject.
*/

QAudioRecorder::QAudioRecorder(QObject *parent):
    QMediaRecorder(*new QAudioRecorderPrivate, nullptr, parent)
{
    Q_D(QAudioRecorder);
    d->provider = QMediaServiceProvider::defaultServiceProvider();

    QMediaService *service = d->provider->requestService(Q_MEDIASERVICE_AUDIOSOURCE);
    setMediaObject(new QAudioRecorderObject(this, service));
}

/*!
    Destroys an audio recorder object.
*/

QAudioRecorder::~QAudioRecorder()
{
    Q_D(QAudioRecorder);
    QMediaService *service = d->mediaObject ? d->mediaObject->service() : nullptr;
    setMediaObject(nullptr);

    if (d->provider && service)
        d->provider->releaseService(service);
}

QT_END_NAMESPACE

#include "moc_qaudiorecorder.cpp"
