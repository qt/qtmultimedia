// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXSNDAUDIODEVICES_P_H
#define QQNXSNDAUDIODEVICES_P_H

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
#include <QtMultimedia/qaudio.h>

QT_BEGIN_NAMESPACE

class QQnxSndAudioDevices final : public QPlatformAudioDevices
{
public:
    QQnxSndAudioDevices();

    QPlatformAudioSource *createAudioSource(const QAudioDevice &, const QAudioFormat &,
                                            QObject *parent) override;
    QPlatformAudioSink *createAudioSink(const QAudioDevice &, const QAudioFormat &,
                                        QObject *parent) override;

    QLatin1String backendName() const override { return QLatin1String{ "QNX-snd" }; }

    bool hasCallbackApi() const override { return true; }

protected:
    QList<QAudioDevice> findAudioInputs() const override;
    QList<QAudioDevice> findAudioOutputs() const override;
};

QT_END_NAMESPACE

#endif // QQNXSNDAUDIODEVICES_P_H
