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

#ifndef QCAMERA_P_H
#define QCAMERA_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmediaobject_p.h"
#include "qvideosurfaceoutput_p.h"
#include "qcamera.h"

QT_BEGIN_NAMESPACE

class QMediaServiceProvider;
class QCameraControl;
class QVideoDeviceSelectorControl;
class QCameraLocksControl;
class QCameraInfoControl;
class QCameraViewfinderSettingsControl;
class QCameraViewfinderSettingsControl2;

class QCameraPrivate : public QMediaObjectPrivate
{
    Q_DECLARE_NON_CONST_PUBLIC(QCamera)
public:
    QCameraPrivate():
        QMediaObjectPrivate(),
        provider(nullptr),
        control(nullptr),
        deviceControl(nullptr),
        locksControl(nullptr),
        infoControl(nullptr),
        viewfinderSettingsControl(nullptr),
        viewfinderSettingsControl2(nullptr),
        cameraExposure(nullptr),
        cameraFocus(nullptr),
        imageProcessing(nullptr),
        viewfinder(nullptr),
        capture(nullptr),
        state(QCamera::UnloadedState),
        error(QCamera::NoError),
        requestedLocks(QCamera::NoLock),
        lockStatus(QCamera::Unlocked),
        lockChangeReason(QCamera::UserRequest),
        supressLockChangedSignal(false),
        restartPending(false)
    {
    }

    void init();
    void initControls();

    void clear();

    QMediaServiceProvider *provider;

    QCameraControl *control;
    QVideoDeviceSelectorControl *deviceControl;
    QCameraLocksControl *locksControl;
    QCameraInfoControl *infoControl;
    QCameraViewfinderSettingsControl *viewfinderSettingsControl;
    QCameraViewfinderSettingsControl2 *viewfinderSettingsControl2;

    QCameraExposure *cameraExposure;
    QCameraFocus *cameraFocus;
    QCameraImageProcessing *imageProcessing;

    QObject *viewfinder;
    QObject *capture;

    QCamera::State state;

    QCamera::Error error;
    QString errorString;

    QCamera::LockTypes requestedLocks;

    QCamera::LockStatus lockStatus;
    QCamera::LockChangeReason lockChangeReason;
    bool supressLockChangedSignal;

    bool restartPending;

    QVideoSurfaceOutput surfaceViewfinder;

    void _q_error(int error, const QString &errorString);
    void unsetError() { error = QCamera::NoError; errorString.clear(); }

    void setState(QCamera::State);

    void _q_updateLockStatus(QCamera::LockType, QCamera::LockStatus, QCamera::LockChangeReason);
    void _q_updateState(QCamera::State newState);
    void _q_preparePropertyChange(int changeType);
    void _q_restartCamera();
    void updateLockStatus();
};

QT_END_NAMESPACE

#endif // QCAMERA_P_H
