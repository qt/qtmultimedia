// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPULSEAUDIODEVICES_H
#define QPULSEAUDIODEVICES_H

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
#include <qset.h>
#include <qaudio.h>

QT_BEGIN_NAMESPACE

class QPulseAudioEngine;

class QPulseAudioDevices : public QPlatformAudioDevices
{
public:
    QPulseAudioDevices();
    ~QPulseAudioDevices() override;

    QPlatformAudioSource *createAudioSource(const QAudioDevice &deviceInfo,
                                            QObject *parent) override;
    QPlatformAudioSink *createAudioSink(const QAudioDevice &deviceInfo,
                                        QObject *parent) override;

    using QPlatformAudioDevices::onAudioInputsChanged;
    using QPlatformAudioDevices::onAudioOutputsChanged;

protected:
    QList<QAudioDevice> findAudioInputs() const override;
    QList<QAudioDevice> findAudioOutputs() const override;

private:
    QPulseAudioEngine *pulseEngine;
};

QT_END_NAMESPACE

#endif
