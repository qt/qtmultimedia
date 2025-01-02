// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDAUDIOINPUT_H
#define QANDROIDAUDIOINPUT_H

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

#include <QtCore/qobject.h>

#include <private/qtmultimediaglobal_p.h>
#include <private/qplatformaudioinput_p.h>

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIA_EXPORT QAndroidAudioInput : public QObject, public QPlatformAudioInput
{
    Q_OBJECT

public:
    explicit QAndroidAudioInput(QAudioInput *parent);
    ~QAndroidAudioInput();

    void setMuted(bool muted) override;

    bool isMuted() const;

Q_SIGNALS:
    void mutedChanged(bool muted);

private:
    bool m_muted = false;

};

QT_END_NAMESPACE

#endif
