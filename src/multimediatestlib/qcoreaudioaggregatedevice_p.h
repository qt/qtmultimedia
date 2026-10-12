// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QCOREAUDIOAGGREGATEDEVICE_P_H
#define QCOREAUDIOAGGREGATEDEVICE_P_H

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

#include <QtMultimedia/qaudiodevice.h>
#include <QtCore/qstring.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

// Test helper: creates a CoreAudio aggregate device wrapping the system default device, and
// destroys it on request. Destroying the aggregate device looks like an unplugged device to
// QtMultimedia, without needing any hardware to be unplugged.
class QCoreAudioAggregateDevice
{
public:
    enum class Direction : uint8_t {
        Output,
        Input,
    };

    static std::unique_ptr<QCoreAudioAggregateDevice> create(Direction);

    Q_DISABLE_COPY_MOVE(QCoreAudioAggregateDevice)
    ~QCoreAudioAggregateDevice();

    // Polls QMediaDevices until the aggregate shows up. Nullopt on timeout.
    std::optional<QAudioDevice> waitForQtDevice(std::chrono::milliseconds timeout) const;

    bool destroyAndWaitForDisconnectNotification(std::chrono::milliseconds timeout);

private:
    struct Private;
    explicit QCoreAudioAggregateDevice(std::unique_ptr<Private>);

    std::unique_ptr<Private> d;
};

QT_END_NAMESPACE

#endif // QCOREAUDIOAGGREGATEDEVICE_P_H
