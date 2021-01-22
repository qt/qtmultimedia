/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qmediaaudioprobecontrol.h"
#include "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QMediaAudioProbeControl
    \obsolete
    \inmodule QtMultimedia


    \ingroup multimedia_control

    \brief The QMediaAudioProbeControl class allows control over probing audio data in media objects.

    \l QAudioProbe is the client facing class for probing audio - this class is implemented by
    media backends to provide this functionality.

    The interface name of QMediaAudioProbeControl is \c org.qt-project.qt.mediaaudioprobecontrol/5.0 as
    defined in QMediaAudioProbeControl_iid.

    \sa QAudioProbe, QMediaService::requestControl(), QMediaPlayer, QCamera
*/

/*!
    \macro QMediaAudioProbeControl_iid

    \c org.qt-project.qt.mediaaudioprobecontrol/5.0

    Defines the interface name of the QMediaAudioProbeControl class.

    \relates QMediaAudioProbeControl
*/

/*!
  Create a new media audio probe control object with the given \a parent.
*/
QMediaAudioProbeControl::QMediaAudioProbeControl(QObject *parent)
    : QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*! Destroys this audio probe control */
QMediaAudioProbeControl::~QMediaAudioProbeControl()
{
}

/*!
    \fn QMediaAudioProbeControl::audioBufferProbed(const QAudioBuffer &buffer)

    This signal should be emitted when an audio \a buffer is processed in the
    media service.
*/


/*!
    \fn QMediaAudioProbeControl::flush()

    This signal should be emitted when it is required to release all frames.
*/

QT_END_NAMESPACE

#include "moc_qmediaaudioprobecontrol.cpp"
