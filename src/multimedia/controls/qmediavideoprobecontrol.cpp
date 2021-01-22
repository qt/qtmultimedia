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

#include "qmediavideoprobecontrol.h"
#include "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QMediaVideoProbeControl
    \obsolete
    \inmodule QtMultimedia


    \ingroup multimedia_control

    \brief The QMediaVideoProbeControl class allows control over probing video frames in media objects.

    \l QVideoProbe is the client facing class for probing video - this class is implemented by
    media backends to provide this functionality.

    The interface name of QMediaVideoProbeControl is \c org.qt-project.qt.mediavideoprobecontrol/5.0 as
    defined in QMediaVideoProbeControl_iid.

    \sa QVideoProbe, QMediaService::requestControl(), QMediaPlayer, QCamera
*/

/*!
    \macro QMediaVideoProbeControl_iid

    \c org.qt-project.qt.mediavideoprobecontrol/5.0

    Defines the interface name of the QMediaVideoProbeControl class.

    \relates QMediaVideoProbeControl
*/

/*!
  Create a new media video probe control object with the given \a parent.
*/
QMediaVideoProbeControl::QMediaVideoProbeControl(QObject *parent)
    : QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*! Destroys this video probe control */
QMediaVideoProbeControl::~QMediaVideoProbeControl()
{
}

/*!
    \fn QMediaVideoProbeControl::videoFrameProbed(const QVideoFrame &frame)

    This signal should be emitted when a video \a frame is processed in the
    media service.
*/

/*!
    \fn QMediaVideoProbeControl::flush()

    This signal should be emitted when it is required to release all frames.
*/

QT_END_NAMESPACE

#include "moc_qmediavideoprobecontrol.cpp"
