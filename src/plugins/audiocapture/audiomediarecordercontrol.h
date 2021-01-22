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

#ifndef AUDIOMEDIARECORDERCONTROL_H
#define AUDIOMEDIARECORDERCONTROL_H

#include <QtCore/qobject.h>

#include "qmediarecorder.h"
#include "qmediarecordercontrol.h"

QT_BEGIN_NAMESPACE

class AudioCaptureSession;

class AudioMediaRecorderControl : public QMediaRecorderControl
{
    Q_OBJECT
public:
    AudioMediaRecorderControl(QObject *parent = 0);
    ~AudioMediaRecorderControl();

    QUrl outputLocation() const;
    bool setOutputLocation(const QUrl &location);

    QMediaRecorder::State state() const;
    QMediaRecorder::Status status() const;

    qint64 duration() const;

    bool isMuted() const;
    qreal volume() const;

    void applySettings() {}

    void setState(QMediaRecorder::State state);
    void setMuted(bool);
    void setVolume(qreal volume);

private:
    AudioCaptureSession* m_session;
};

QT_END_NAMESPACE

#endif
