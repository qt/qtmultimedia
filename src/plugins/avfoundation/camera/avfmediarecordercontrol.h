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

#ifndef AVFMEDIARECORDERCONTROL_H
#define AVFMEDIARECORDERCONTROL_H

#include <QtCore/qurl.h>
#include <QtMultimedia/qmediarecordercontrol.h>

#import <AVFoundation/AVFoundation.h>
#include "avfstoragelocation.h"

QT_BEGIN_NAMESPACE

class AVFCameraSession;
class AVFCameraControl;
class AVFCameraService;
@class AVFMediaRecorderDelegate;

class AVFMediaRecorderControl : public QMediaRecorderControl
{
Q_OBJECT
public:
    AVFMediaRecorderControl(AVFCameraService *service, QObject *parent = 0);
    ~AVFMediaRecorderControl();

    QUrl outputLocation() const;
    bool setOutputLocation(const QUrl &location);

    QMediaRecorder::State state() const;
    QMediaRecorder::Status status() const;

    qint64 duration() const;

    bool isMuted() const;
    qreal volume() const;

    void applySettings();

public Q_SLOTS:
    void setState(QMediaRecorder::State state);
    void setMuted(bool muted);
    void setVolume(qreal volume);

    void handleRecordingStarted();
    void handleRecordingFinished();
    void handleRecordingFailed(const QString &message);

private Q_SLOTS:
    void reconnectMovieOutput();
    void updateStatus();

private:
    AVFCameraControl *m_cameraControl;
    AVFCameraSession *m_session;

    bool m_connected;
    QUrl m_outputLocation;
    QMediaRecorder::State m_state;
    QMediaRecorder::Status m_lastStatus;

    bool m_recordingStarted;
    bool m_recordingFinished;

    bool m_muted;
    qreal m_volume;

    AVCaptureMovieFileOutput *m_movieOutput;
    AVFMediaRecorderDelegate *m_recorderDelagate;
    AVFStorageLocation m_storageLocation;
};

QT_END_NAMESPACE

#endif
