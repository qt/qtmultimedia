// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QMOCKMEDIADEVICES_H
#define QMOCKMEDIADEVICES_H

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
#include <qaudiodevice.h>
#include <qcameradevice.h>

QT_BEGIN_NAMESPACE

class QCameraDevice;

class QMockAudioDevices : public QPlatformAudioDevices
{
public:
    QMockAudioDevices();
    ~QMockAudioDevices();

    QPlatformAudioSource *createAudioSource(const QAudioDevice &info, QObject *parent) override;
    QPlatformAudioSink *createAudioSink(const QAudioDevice &info, QObject *parent) override;

protected:
    QList<QAudioDevice> findAudioInputs() const override;
    QList<QAudioDevice> findAudioOutputs() const override;

private:
    QList<QAudioDevice> m_inputDevices;
    QList<QAudioDevice> m_outputDevices;
};

QT_END_NAMESPACE

#endif
