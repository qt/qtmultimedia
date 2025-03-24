// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_AUDIOSOURCE_P_H
#define QPIPEWIRE_AUDIOSOURCE_P_H

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

#include "qpipewire_support_p.h"

#include "qpipewire_audioiobase_p.h"

#include <QtMultimedia/private/qaudiosystem_p.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

class QPipewireAudioDevicePrivate;
struct QPipewireAudioSourceStream;

class QPipewireAudioSource final
    : public QPipewireAudioIOBase<QPlatformAudioSource, QPipewireAudioSourceStream>
{
    using SampleFormat = QAudioFormat::SampleFormat;
    using BaseClass = QPipewireAudioIOBase<QPlatformAudioSource, QPipewireAudioSourceStream>;

public:
    QPipewireAudioSource(QAudioDevice, const QAudioFormat &format, QObject *parent);
    ~QPipewireAudioSource() override;

    // QPlatformAudioSource interface
    void start(QIODevice *device) override;
    QIODevice *start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesReady() const override;

private:
    friend struct QPipewireAudioSourceStream;

    std::optional<ObjectSerial> findSourceNodeSerial();

    template <typename Functor>
    void startHelper(Functor &&f);

    void reportXRuns(int numberOfXruns);

    std::shared_ptr<QPipewireAudioSourceStream> m_retiredStream;
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif
