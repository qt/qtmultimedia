// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_AUDIOSINK_P_H
#define QPIPEWIRE_AUDIOSINK_P_H

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

#include "qpipewire_audioiobase_p.h"

#include <QtMultimedia/private/qaudiosystem_p.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

class QPipewireAudioDevicePrivate;
struct QPipewireAudioSinkStream;

class QPipewireAudioSink final
    : public QPipewireAudioIOBase<QPlatformAudioSink, QPipewireAudioSinkStream>
{
    using SampleFormat = QAudioFormat::SampleFormat;
    using BaseClass = QPipewireAudioIOBase<QPlatformAudioSink, QPipewireAudioSinkStream>;

public:
    QPipewireAudioSink(QAudioDevice, const QAudioFormat &format, QObject *parent);
    ~QPipewireAudioSink() override;
    Q_DISABLE_COPY_MOVE(QPipewireAudioSink)

    void start(QIODevice *device) override;
    QIODevice *start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesFree() const override;

    using QPlatformAudioEndpointBase::updateStreamState;

private:
    friend class QtMultimediaPrivate::QPlatformAudioSinkStream;
    friend QPipewireAudioSinkStream;
    using BaseClass::m_stream;

    void reportXRuns(int);

    template <typename Functor>
    void startHelper(Functor &&f);

    void setRole(AudioEndpointRole role) override;
    AudioEndpointRole m_role = AudioEndpointRole::Other;
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif
