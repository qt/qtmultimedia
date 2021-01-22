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


#ifndef QGSTREAMERCAMERACONTROL_H
#define QGSTREAMERCAMERACONTROL_H

#include <QHash>
#include <qcameracontrol.h>
#include "qgstreamercapturesession.h"

QT_BEGIN_NAMESPACE

class QGstreamerCameraControl : public QCameraControl
{
    Q_OBJECT
public:
    QGstreamerCameraControl( QGstreamerCaptureSession *session );
    virtual ~QGstreamerCameraControl();

    bool isValid() const { return true; }

    QCamera::State state() const override;
    void setState(QCamera::State state) override;

    QCamera::Status status() const override { return m_status; }

    QCamera::CaptureModes captureMode() const override { return m_captureMode; }
    void setCaptureMode(QCamera::CaptureModes mode) override;

    bool isCaptureModeSupported(QCamera::CaptureModes mode) const override;

    QCamera::LockTypes supportedLocks() const
    {
        return QCamera::NoLock;
    }

    bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const override;

public slots:
    void reloadLater();

private slots:
    void updateStatus();
    void reloadPipeline();


private:
    QCamera::CaptureModes m_captureMode;
    QGstreamerCaptureSession *m_session;
    QCamera::State m_state;
    QCamera::Status m_status;
    bool m_reloadPending;
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAMERACONTROL_H
