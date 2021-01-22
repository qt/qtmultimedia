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


#ifndef CAMERABINRECORDERCONTROL_H
#define CAMERABINRECORDERCONTROL_H

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <qmediarecordercontrol.h>
#include "camerabinsession.h"

#if QT_CONFIG(gstreamer_encodingprofiles)
#include <gst/pbutils/encoding-profile.h>
#endif

QT_BEGIN_NAMESPACE

class CameraBinRecorder : public QMediaRecorderControl
{
    Q_OBJECT

public:
    CameraBinRecorder(CameraBinSession *session);
    virtual ~CameraBinRecorder();

    QUrl outputLocation() const override;
    bool setOutputLocation(const QUrl &sink) override;

    QMediaRecorder::State state() const override;
    QMediaRecorder::Status status() const override;

    qint64 duration() const override;

    bool isMuted() const override;
    qreal volume() const override;

    void applySettings() override;

#if QT_CONFIG(gstreamer_encodingprofiles)
    GstEncodingContainerProfile *videoProfile();
#endif

public slots:
    void setState(QMediaRecorder::State state) override;
    void setMuted(bool) override;
    void setVolume(qreal volume) override;

    void updateStatus();

private:
    CameraBinSession *m_session;
    QMediaRecorder::State m_state;
    QMediaRecorder::Status m_status;
};

QT_END_NAMESPACE

#endif // CAMERABINCAPTURECORNTROL_H
