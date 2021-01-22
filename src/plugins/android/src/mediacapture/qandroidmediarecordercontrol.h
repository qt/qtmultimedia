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

#ifndef QANDROIDMEDIARECORDERCONTROL_H
#define QANDROIDMEDIARECORDERCONTROL_H

#include <qmediarecordercontrol.h>

QT_BEGIN_NAMESPACE

class QAndroidCaptureSession;

class QAndroidMediaRecorderControl : public QMediaRecorderControl
{
    Q_OBJECT
public:
    explicit QAndroidMediaRecorderControl(QAndroidCaptureSession *session);

    QUrl outputLocation() const override;
    bool setOutputLocation(const QUrl &location) override;
    QMediaRecorder::State state() const override;
    QMediaRecorder::Status status() const override;
    qint64 duration() const override;
    bool isMuted() const override;
    qreal volume() const override;
    void applySettings() override;

public Q_SLOTS:
    void setState(QMediaRecorder::State state) override;
    void setMuted(bool muted) override;
    void setVolume(qreal volume) override;

private:
    QAndroidCaptureSession *m_session;
};

QT_END_NAMESPACE

#endif // QANDROIDMEDIARECORDERCONTROL_H
