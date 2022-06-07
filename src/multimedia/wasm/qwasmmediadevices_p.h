// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMMEDIADEVICES_H
#define QWASMMEDIADEVICES_H

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

#include <private/qplatformmediadevices_p.h>
#include <qset.h>
#include <qaudio.h>
#include <qaudiodevice.h>

QT_BEGIN_NAMESPACE

class QWasmAudioEngine;

class QWasmMediaDevices : public QPlatformMediaDevices
{
public:
    QWasmMediaDevices();

    QList<QAudioDevice> audioInputs() const override;
    QList<QAudioDevice> audioOutputs() const override;
    QList<QCameraDevice> videoInputs() const override;
    QPlatformAudioSource *createAudioSource(const QAudioDevice &deviceInfo) override;
    QPlatformAudioSink *createAudioSink(const QAudioDevice &deviceInfo) override;

private:
    QList<QAudioDevice> m_outs;
    QList<QAudioDevice> m_ins;
};

QT_END_NAMESPACE

#endif
