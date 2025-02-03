// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_AUDIOSTREAM_P_H
#define QPIPEWIRE_AUDIOSTREAM_P_H

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

#include <QtCore/qglobal.h>
#include "QtMultimedia/qaudioformat.h"

#include "qpipewire_audiodevicemonitor_p.h"
#include "qpipewire_support_p.h"

#include <pipewire/stream.h>

#include <array>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

struct QPipewireAudioStream
{
protected:
    explicit QPipewireAudioStream(const QAudioFormat &);
    ~QPipewireAudioStream();

    void prepareParameters();
    const QAudioFormat m_format;

    // stream control
    void createStream(QSpan<spa_dict_item> extraProperties,
                      std::optional<qsizetype> hardwareBufferSize, const char *streamName);
    bool connectStream(ObjectSerial target, spa_direction);
    void disconnectStream();

public:
    void suspend();
    void resume();
    bool hasStream() const;

protected:
    // stream callbacks
    virtual void process() = 0;
    virtual void stateChanged(pw_stream_state oldState, pw_stream_state state,
                              const char *error) = 0;

    // stream members
    std::array<uint8_t, 1024> parameterBuffer;
    std::array<const struct spa_pod *, 1> params;
    pw_stream_events stream_events{};
    PwStreamHandle m_stream;

    // device observer
    [[nodiscard]] bool registerDeviceObserver(ObjectSerial);
    void unregisterDeviceObserver();
    virtual void handleDeviceRemoved() = 0;
    SharedObjectRemoveObserver m_deviceRemovalObserver;
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif
