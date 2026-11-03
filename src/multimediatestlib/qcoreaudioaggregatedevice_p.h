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

    // CoreAudio refuses to make a private aggregate device the system default
    enum class Visibility : uint8_t {
        Private,
        Public,
    };

    // The first call in a process first removes the stale devices of earlier processes
    static std::unique_ptr<QCoreAudioAggregateDevice> create(Direction,
                                                             Visibility = Visibility::Private);

    // Same name and UID for every call with the same identity, so a re-created aggregate is
    // the same QAudioDevice as the one that was destroyed.
    static std::unique_ptr<QCoreAudioAggregateDevice> create(Direction, const QString &identity,
                                                             Visibility = Visibility::Private);

    // The UID records the owning process. A device whose owner exited was left behind by a
    // crashed test run, so it is destroyed. Destroying it makes the default fall back.
    static void removeStaleDevices();

    // Creates a device as if owned by another process
    static std::unique_ptr<QCoreAudioAggregateDevice>
    createOwnedBy(Direction, qint64 ownerPid, const QString &identity,
                  Visibility = Visibility::Private);

    Q_DISABLE_COPY_MOVE(QCoreAudioAggregateDevice)
    ~QCoreAudioAggregateDevice();

    // Polls QMediaDevices until the aggregate shows up. Nullopt on timeout.
    std::optional<QAudioDevice> waitForQtDevice(std::chrono::milliseconds timeout) const;

    // Makes the aggregate the system default. Returns false if CoreAudio keeps the previous
    // default, which is what happens for private devices. The previous default is restored
    // when this object is destroyed.
    bool makeSystemDefault();

    bool destroyAndWaitForDisconnectNotification(std::chrono::milliseconds timeout);

private:
    struct Private;
    explicit QCoreAudioAggregateDevice(std::unique_ptr<Private>);

    std::unique_ptr<Private> d;
};

QT_END_NAMESPACE

#endif // QCOREAUDIOAGGREGATEDEVICE_P_H
