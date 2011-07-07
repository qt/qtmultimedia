/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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

#ifndef S60MEDIARECORDERCONTROL_H
#define S60MEDIARECORDERCONTROL_H

#include <QtCore/qurl.h>
#include <qmediarecorder.h>
#include <qmediarecordercontrol.h>

#include "s60videocapturesession.h"

QT_USE_NAMESPACE

class S60VideoCaptureSession;
class S60CameraService;
class S60CameraControl;

/*
 * Control for video recording operations.
 */
class S60MediaRecorderControl : public QMediaRecorderControl
{
    Q_OBJECT

public: // Contructors & Destructor

    S60MediaRecorderControl(QObject *parent = 0);
    S60MediaRecorderControl(S60CameraService *service,
                            S60VideoCaptureSession *session,
                            QObject *parent = 0);
    ~S60MediaRecorderControl();

public: // QMediaRecorderControl

    QUrl outputLocation() const;
    bool setOutputLocation(const QUrl &sink);

    QMediaRecorder::State state() const;

    qint64 duration() const;

    bool isMuted() const;

    void applySettings();

/*
Q_SIGNALS: // QMediaRecorderControl
    void stateChanged(QMediaRecorder::State state);
    void durationChanged(qint64 position);
    void mutedChanged(bool muted);
    void error(int error, const QString &errorString);
*/

public slots: // QMediaRecorderControl

    void record();
    void pause();
    void stop();
    void setMuted(bool);

private:

    QMediaRecorder::State convertInternalStateToQtState(
        S60VideoCaptureSession::TVideoCaptureState aState) const;

private slots:

    void updateState(S60VideoCaptureSession::TVideoCaptureState state);

private: // Data

    S60VideoCaptureSession  *m_session;
    S60CameraService        *m_service;
    S60CameraControl        *m_cameraControl;
    QMediaRecorder::State   m_state;

};

#endif // S60MEDIARECORDERCONTROL_H
