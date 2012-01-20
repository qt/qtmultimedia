/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMEDIARECORDER_P_H
#define QMEDIARECORDER_P_H

#include "qmediarecorder.h"
#include "qmediaobject_p.h"

QT_BEGIN_NAMESPACE

class QMediaRecorderControl;
class QMediaContainerControl;
class QAudioEncoderControl;
class QVideoEncoderControl;
class QMetaDataWriterControl;
class QTimer;

class QMediaRecorderPrivate
{
    Q_DECLARE_NON_CONST_PUBLIC(QMediaRecorder)

public:
    QMediaRecorderPrivate();
    virtual ~QMediaRecorderPrivate() {}

    QMediaObject *mediaObject;

    QMediaRecorderControl *control;
    QMediaContainerControl *formatControl;
    QAudioEncoderControl *audioControl;
    QVideoEncoderControl *videoControl;
    QMetaDataWriterControl *metaDataControl;

    QTimer* notifyTimer;

    QMediaRecorder::State state;
    QMediaRecorder::Error error;
    QString errorString;

    void _q_stateChanged(QMediaRecorder::State state);
    void _q_error(int error, const QString &errorString);
    void _q_serviceDestroyed();
    void _q_notify();
    void _q_updateNotifyInterval(int ms);

    QMediaRecorder *q_ptr;
};

QT_END_NAMESPACE

#endif

