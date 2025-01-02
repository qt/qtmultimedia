// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXAUDIOINPUT_P_H
#define QQNXAUDIOINPUT_P_H

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
#include <private/qplatformaudioinput_p.h>

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIA_EXPORT QQnxAudioInput : public QPlatformAudioInput
{
public:
    explicit QQnxAudioInput(QAudioInput *parent);
    ~QQnxAudioInput();

    void setAudioDevice(const QAudioDevice &device) override;
};

QT_END_NAMESPACE
#endif
