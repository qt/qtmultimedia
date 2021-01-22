/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QDECLARATIVECAMERAFLASH_H
#define QDECLARATIVECAMERAFLASH_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qcamera.h>
#include <qcameraexposure.h>

QT_BEGIN_NAMESPACE

class QDeclarativeCamera;

class QDeclarativeCameraFlash : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isFlashReady NOTIFY flashReady)
    Q_PROPERTY(FlashMode mode READ flashMode WRITE setFlashMode NOTIFY flashModeChanged)
    Q_PROPERTY(QVariantList supportedModes READ supportedModes NOTIFY supportedModesChanged REVISION 1)

    Q_ENUMS(FlashMode)
public:
    enum FlashMode {
        FlashAuto = QCameraExposure::FlashAuto,
        FlashOff = QCameraExposure::FlashOff,
        FlashOn = QCameraExposure::FlashOn,
        FlashRedEyeReduction = QCameraExposure::FlashRedEyeReduction,
        FlashFill = QCameraExposure::FlashFill,
        FlashTorch = QCameraExposure::FlashTorch,
        FlashVideoLight = QCameraExposure::FlashVideoLight,
        FlashSlowSyncFrontCurtain = QCameraExposure::FlashSlowSyncFrontCurtain,
        FlashSlowSyncRearCurtain = QCameraExposure::FlashSlowSyncRearCurtain,
        FlashManual = QCameraExposure::FlashManual
    };

    ~QDeclarativeCameraFlash();

    FlashMode flashMode() const;
    bool isFlashReady() const;
    QVariantList supportedModes() const;

public Q_SLOTS:
    void setFlashMode(FlashMode);

Q_SIGNALS:
    void flashReady(bool status);
    void flashModeChanged(FlashMode);
    void supportedModesChanged();

private slots:
    void _q_cameraStatusChanged(QCamera::Status status);

private:
    friend class QDeclarativeCamera;
    QDeclarativeCameraFlash(QCamera *camera, QObject *parent = 0);

    QCameraExposure *m_exposure;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QT_PREPEND_NAMESPACE(QDeclarativeCameraFlash))

#endif
