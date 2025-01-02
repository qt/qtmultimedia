// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPULSEAUDIOMEDIADEVICES_H
#define QPULSEAUDIOMEDIADEVICES_H

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

QT_BEGIN_NAMESPACE

class QPulseAudioEngine;

class QPulseAudioMediaDevices : public QPlatformMediaDevices
{
public:
    QPulseAudioMediaDevices();
    ~QPulseAudioMediaDevices();

    QList<QAudioDevice> audioInputs() const override;
    QList<QAudioDevice> audioOutputs() const override;
    QPlatformAudioSource *createAudioSource(const QAudioDevice &deviceInfo,
                                            QObject *parent) override;
    QPlatformAudioSink *createAudioSink(const QAudioDevice &deviceInfo,
                                        QObject *parent) override;

private:
    QPulseAudioEngine *pulseEngine;
};

QT_END_NAMESPACE

#endif
