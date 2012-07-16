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


#ifndef CAMERABINRECORDERCONTROL_H
#define CAMERABINRECORDERCONTROL_H

#include <qmediarecordercontrol.h>
#include "camerabinsession.h"
#include <gst/pbutils/encoding-profile.h>

QT_BEGIN_NAMESPACE

class CameraBinRecorder : public QMediaRecorderControl
{
    Q_OBJECT

public:
    CameraBinRecorder(CameraBinSession *session);
    virtual ~CameraBinRecorder();

    QUrl outputLocation() const;
    bool setOutputLocation(const QUrl &sink);

    QMediaRecorder::State state() const;
    QMediaRecorder::Status status() const;

    qint64 duration() const;

    bool isMuted() const;

    void applySettings();
    GstEncodingContainerProfile *videoProfile();

public slots:
    void setState(QMediaRecorder::State state);
    void setMuted(bool);

private slots:
    void updateStatus();

private:
    CameraBinSession *m_session;
    QMediaRecorder::State m_state;
    QMediaRecorder::Status m_status;
};

QT_END_NAMESPACE

#endif // CAMERABINCAPTURECORNTROL_H
