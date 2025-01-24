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

#include "qpipewire_support_p.h"

#include <QtMultimedia/private/qaudiosystem_p.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

class QPipewireAudioDevicePrivate;
struct QPipewireAudioSinkStream;

class QPipewireAudioSink final : public QPlatformAudioSink
{
    using SampleFormat = QAudioFormat::SampleFormat;

public:
    QPipewireAudioSink(const QAudioDevice &, QObject *parent);
    ~QPipewireAudioSink() override;

    void start(QIODevice *device) override;
    QIODevice *start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesFree() const override;
    void setBufferSize(qsizetype value) override;
    qsizetype bufferSize() const override;
    qint64 processedUSecs() const override;
    QtAudio::Error error() const override;
    QtAudio::State state() const override;
    void setFormat(const QAudioFormat &format) override;
    QAudioFormat format() const override;

    void setVolume(qreal volume) override;
    qreal volume() const override;

private:
    friend QPipewireAudioSinkStream;
    void reportXRuns(int);

    QAudioDevice m_audioDevice;
    QAudioFormat m_format;
    qreal m_volume = 1.0f;
    std::optional<qsizetype> m_bufferSize;
    std::optional<qsizetype> m_hardwareBufferSize;

    const QPipewireAudioDevicePrivate *privateDevice();
    std::optional<ObjectSerial> findSinkNodeSerial();

    template <typename Functor>
    void startHelper(Functor &&f);

    // errors
    void updateError(QAudio::Error);
    QAudio::Error m_error = QAudio::Error::NoError;

    // streams
    std::shared_ptr<QPipewireAudioSinkStream> m_stream;

    // state
    void setUserOwnedState(QtAudio::State);
    QtAudio::State m_userOwnedState = QtAudio::StoppedState;
    void streamIdle(bool);
    bool m_streamIsIdle = false;
    void updateState();
    QtAudio::State m_state = QtAudio::StoppedState;
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif
