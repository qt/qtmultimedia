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

#include "qmediavideoprobecontrol.h"
#include "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QMediaVideoProbeControl
    \inmodule QtMultimedia

    \ingroup multimedia
    \ingroup multimedia_control

    \brief The QMediaVideoProbeControl class allows control over probing video frames in media objects.

    \l QVideoProbe is the client facing class for probing video - this class is implemented by
    media backends to provide this functionality.

    The interface name of QMediaVideoProbeControl is \c com.nokia.Qt.QMediaVideoProbeControl/1.0 as
    defined in QMediaVideoProbeControl_iid.

    \sa QVideoProbe, QMediaService::requestControl(), QMediaPlayer, QCamera
*/

/*!
    \macro QMediaVideoProbeControl_iid

    \c com.nokia.Qt.QMediaVideoProbeControl/1.0

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

    This signal should be emitted when a video frame is processed in the
    media service.
*/

/*!
    \fn QMediaVideoProbeControl::flush()

    This signal should be emitted when it is required to release all frames.
*/

#include "moc_qmediavideoprobecontrol.cpp"

QT_END_NAMESPACE
