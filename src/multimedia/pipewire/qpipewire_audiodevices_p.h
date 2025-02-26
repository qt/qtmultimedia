// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_AUDIODEVICES_P_H
#define QPIPEWIRE_AUDIODEVICES_P_H

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

#include <QtCore/qlist.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/private/qplatformaudiodevices_p.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

class QAudioDevices : public QPlatformAudioDevices
{
public:
    QAudioDevices();
    ~QAudioDevices() override = default;

    static bool isSupported();

    QList<QAudioDevice> findAudioInputs() const override;
    QList<QAudioDevice> findAudioOutputs() const override;

    QPlatformAudioSource *createAudioSource(const QAudioDevice &, const QAudioFormat &,
                                            QObject *parent) override;

    QPlatformAudioSink *createAudioSink(const QAudioDevice &, const QAudioFormat &,
                                        QObject *parent) override;

    QLatin1String backendName() const override { return QLatin1String{ "PipeWire" }; }

private:
    QList<QAudioDevice> m_sourceDeviceList;
    QList<QAudioDevice> m_sinkDeviceList;
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_AUDIODEVICES_P_H
