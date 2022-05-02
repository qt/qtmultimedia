/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
******************************************************************************/
#ifndef BBCAMERAEXPOSURECONTROL_H
#define BBCAMERAEXPOSURECONTROL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qplatformcameraexposure_p.h>

QT_BEGIN_NAMESPACE

class BbCameraSession;

class BbCameraExposureControl : public QPlatformCameraExposure
{
    Q_OBJECT
public:
    explicit BbCameraExposureControl(BbCameraSession *session, QObject *parent = 0);

    bool isParameterSupported(ExposureParameter parameter) const override;
    QVariantList supportedParameterRange(ExposureParameter parameter, bool *continuous) const override;

    QVariant requestedValue(ExposureParameter parameter) const override;
    QVariant actualValue(ExposureParameter parameter) const override;
    bool setValue(ExposureParameter parameter, const QVariant& value) override;

    QCamera::FlashMode flashMode() const override;
    void setFlashMode(QCamera::FlashMode mode) override;
    bool isFlashModeSupported(QCamera::FlashMode mode) const override;
    bool isFlashReady() const override;

private Q_SLOTS:
    void statusChanged(QCamera::Status status);

private:
    BbCameraSession *m_session;
    QCamera::ExposureMode m_requestedExposureMode;

    QCamera::FlashMode m_flashMode = QCamera::FlashAuto;
};

QT_END_NAMESPACE

#endif
