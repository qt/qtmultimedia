// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXAUDIOOUTPUT_P_H
#define QQNXAUDIOOUTPUT_P_H

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

#include <private/qtmultimediaglobal_p.h>
#include <private/qplatformaudiooutput_p.h>

QT_BEGIN_NAMESPACE

class QAudioDevice;
class QAudioOutput;

class Q_MULTIMEDIA_EXPORT QQnxAudioOutput : public QPlatformAudioOutput
{
public:
    explicit QQnxAudioOutput(QAudioOutput *parent);
    ~QQnxAudioOutput();

    void setAudioDevice(const QAudioDevice &) override;
    void setVolume(float volume) override;
    void setMuted(bool muted) override;
};

QT_END_NAMESPACE

#endif
