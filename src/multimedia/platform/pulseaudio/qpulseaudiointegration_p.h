/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#ifndef QPULSEAUDIOINTEGRATION_H
#define QPULSEAUDIOINTEGRATION_H

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

#include <private/qplatformmediaintegration_p.h>
#include <private/qaudioengine_pulse_p.h>

QT_BEGIN_NAMESPACE

class QPulseAudioMediaDevices;

class QPulseAudioIntegration : public QPlatformMediaIntegration
{
public:
    QPulseAudioIntegration();
    ~QPulseAudioIntegration();

    QPlatformMediaDevices *devices() override;
    QPlatformMediaFormatInfo *formatInfo() override;

    QPulseAudioMediaDevices *m_devices = nullptr;
    QPulseAudioEngine *pulseEngine = nullptr;
};

QT_END_NAMESPACE

#endif
