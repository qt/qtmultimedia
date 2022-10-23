// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMAUDIOOUTPUT_H
#define QWASMAUDIOOUTPUT_H

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

#include <private/qplatformaudiooutput_p.h>

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIA_EXPORT QWasmAudioOutput : public QPlatformAudioOutput
{
public:
    QWasmAudioOutput(QAudioOutput *qq);
    ~QWasmAudioOutput();

    void setAudioDevice(const QAudioDevice &device);
    void setMuted(bool muted);
    void setVolume(float volume);
};

QT_END_NAMESPACE

#endif // QWASMAUDIOOUTPUT_H
