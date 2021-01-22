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


#ifndef CAMERABINCONTROL_H
#define CAMERABINCONTROL_H

#include <QHash>
#include <qcameracontrol.h>
#include "camerabinsession.h"

QT_BEGIN_NAMESPACE

class CamerabinResourcePolicy;

class CameraBinControl : public QCameraControl
{
    Q_OBJECT
    Q_PROPERTY(bool viewfinderColorSpaceConversion READ viewfinderColorSpaceConversion WRITE setViewfinderColorSpaceConversion)
public:
    CameraBinControl( CameraBinSession *session );
    virtual ~CameraBinControl();

    bool isValid() const { return true; }

    QCamera::State state() const override;
    void setState(QCamera::State state) override;

    QCamera::Status status() const override;

    QCamera::CaptureModes captureMode() const override;
    void setCaptureMode(QCamera::CaptureModes mode) override;

    bool isCaptureModeSupported(QCamera::CaptureModes mode) const override;
    bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const override;
    bool viewfinderColorSpaceConversion() const;

    CamerabinResourcePolicy *resourcePolicy() { return m_resourcePolicy; }

public slots:
    void reloadLater();
    void setViewfinderColorSpaceConversion(bool enabled);

private slots:
    void delayedReload();

    void handleResourcesGranted();
    void handleResourcesLost();

    void handleBusyChanged(bool);
    void handleCameraError(int error, const QString &errorString);

private:
    void updateSupportedResolutions(const QString &device);

    CameraBinSession *m_session;
    QCamera::State m_state;
    CamerabinResourcePolicy *m_resourcePolicy;

    bool m_reloadPending;
};

QT_END_NAMESPACE

#endif // CAMERABINCONTROL_H
