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

#ifndef QMEDIARECORDERCONTROL_H
#define QMEDIARECORDERCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qmediarecorder.h>

QT_BEGIN_NAMESPACE

class QUrl;
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QMediaRecorderControl : public QMediaControl
{
    Q_OBJECT

public:
    virtual ~QMediaRecorderControl();

    virtual QUrl outputLocation() const = 0;
    virtual bool setOutputLocation(const QUrl &location) = 0;

    virtual QMediaRecorder::State state() const = 0;
    virtual QMediaRecorder::Status status() const = 0;

    virtual qint64 duration() const = 0;

    virtual bool isMuted() const = 0;
    virtual qreal volume() const = 0;

    virtual void applySettings() = 0;

Q_SIGNALS:
    void stateChanged(QMediaRecorder::State state);
    void statusChanged(QMediaRecorder::Status status);
    void durationChanged(qint64 position);
    void mutedChanged(bool muted);
    void volumeChanged(qreal volume);
    void actualLocationChanged(const QUrl &location);
    void error(int error, const QString &errorString);

public Q_SLOTS:
    virtual void setState(QMediaRecorder::State state) = 0;
    virtual void setMuted(bool muted) = 0;
    virtual void setVolume(qreal volume) = 0;

protected:
    explicit QMediaRecorderControl(QObject *parent = nullptr);
};

#define QMediaRecorderControl_iid "org.qt-project.qt.mediarecordercontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QMediaRecorderControl, QMediaRecorderControl_iid)

QT_END_NAMESPACE


#endif
