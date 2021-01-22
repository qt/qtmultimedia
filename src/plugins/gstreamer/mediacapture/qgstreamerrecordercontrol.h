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


#ifndef QGSTREAMERRECORDERCONTROL_H
#define QGSTREAMERRECORDERCONTROL_H

#include <QtCore/QDir>

#include <qmediarecordercontrol.h>
#include "qgstreamercapturesession.h"

QT_BEGIN_NAMESPACE

class QGstreamerRecorderControl : public QMediaRecorderControl
{
    Q_OBJECT

public:
    QGstreamerRecorderControl(QGstreamerCaptureSession *session);
    virtual ~QGstreamerRecorderControl();

    QUrl outputLocation() const override;
    bool setOutputLocation(const QUrl &sink) override;

    QMediaRecorder::State state() const override;
    QMediaRecorder::Status status() const override;

    qint64 duration() const override;

    bool isMuted() const override;
    qreal volume() const override;

    void applySettings() override;

public slots:
    void setState(QMediaRecorder::State state) override;
    void record();
    void pause();
    void stop();
    void setMuted(bool) override;
    void setVolume(qreal volume) override;

private slots:
    void updateStatus();
    void handleSessionError(int code, const QString &description);

private:
    QDir defaultDir() const;
    QString generateFileName(const QDir &dir, const QString &ext) const;

    QUrl m_outputLocation;
    QGstreamerCaptureSession *m_session;
    QMediaRecorder::State m_state;
    QMediaRecorder::Status m_status;
    bool m_hasPreviewState;
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAPTURECORNTROL_H
