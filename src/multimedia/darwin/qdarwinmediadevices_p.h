// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDARWINMEDIADEVICES_H
#define QDARWINMEDIADEVICES_H

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
#include <qelapsedtimer.h>
#include <qcameradevice.h>

QT_BEGIN_NAMESPACE

class QCameraDevice;

class QDarwinMediaDevices : public QPlatformMediaDevices
{
public:
    QDarwinMediaDevices();
    ~QDarwinMediaDevices() override;

    QPlatformAudioSource *createAudioSource(const QAudioDevice &info,
                                            QObject *parent) override;
    QPlatformAudioSink *createAudioSink(const QAudioDevice &info,
                                        QObject *parent) override;

    void onInputsUpdated();
    void onOutputsUpdated();

protected:
    QList<QAudioDevice> findAudioInputs() const override;
    QList<QAudioDevice> findAudioOutputs() const override;

private:
    QList<QAudioDevice> m_cachedAudioInputs; // To be removed
    QList<QAudioDevice> m_cachedAudioOutputs; // To be removed
};

QT_END_NAMESPACE

#endif
