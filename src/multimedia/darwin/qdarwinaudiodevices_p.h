// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDARWINAUDIODEVICES_H
#define QDARWINAUDIODEVICES_H

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

#include <private/qplatformaudiodevices_p.h>
#include <qelapsedtimer.h>
#include <qcameradevice.h>

QT_BEGIN_NAMESPACE

class QCameraDevice;

class QDarwinAudioDevices : public QPlatformAudioDevices
{
public:
    QDarwinAudioDevices();
    ~QDarwinAudioDevices() override;

    QPlatformAudioSource *createAudioSource(const QAudioDevice &, const QAudioFormat &,
                                            QObject *parent) override;
    QPlatformAudioSink *createAudioSink(const QAudioDevice &, const QAudioFormat &,
                                        QObject *parent) override;

    using QPlatformAudioDevices::updateAudioInputsCache;
    using QPlatformAudioDevices::updateAudioOutputsCache;

    QLatin1String backendName() const override { return QLatin1String{ "CoreAudio" }; }

protected:
    QList<QAudioDevice> findAudioInputs() const override;
    QList<QAudioDevice> findAudioOutputs() const override;
};

QT_END_NAMESPACE

#endif // QDARWINAUDIODEVICES_H
