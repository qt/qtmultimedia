// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOREAUDIODEVICES_H
#define QCOREAUDIODEVICES_H

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

#include <private/qplatformaudiodevices_p.h>

#ifdef Q_OS_MACOS
#  include <QtCore/qfuture.h>
#  include <CoreAudio/AudioHardware.h>
#  include <dispatch/dispatch.h>
#  include <optional>
#endif

QT_BEGIN_NAMESPACE

class QCoreAudioDevices : public QPlatformAudioDevices
{
public:
    QCoreAudioDevices();
    ~QCoreAudioDevices() override;

    QPlatformAudioSource *createAudioSource(const QAudioDevice &, const QAudioFormat &,
                                            QObject *parent) override;
    QPlatformAudioSink *createAudioSink(const QAudioDevice &, const QAudioFormat &,
                                        QObject *parent) override;

    using QPlatformAudioDevices::updateAudioInputsCache;
    using QPlatformAudioDevices::updateAudioOutputsCache;

    QLatin1String backendName() const override { return QLatin1String{ "CoreAudio" }; }
    bool hasCallbackApi() const override { return true; }

private:
    QList<QAudioDevice> findAudioInputs() const override;
    QList<QAudioDevice> findAudioOutputs() const override;
#ifdef Q_OS_MACOS
    struct DispatchQueueDeleter
    {
        void operator()(dispatch_queue_t queue) const
        {
            if (queue)
                dispatch_release(queue);
        }
    };
    using UniqueDispatchQueue =
            std::unique_ptr<std::remove_pointer_t<dispatch_queue_t>, DispatchQueueDeleter>;

    std::unique_ptr<AudioObjectPropertyListenerBlock> m_deviceListenerBlock;
    UniqueDispatchQueue m_listenerQueue;
    std::shared_ptr<bool> m_destroyed = std::make_shared<bool>(false);
#endif
};

namespace QCoreAudioUtils {

#ifdef Q_OS_MACOS
class DeviceDisconnectMonitor final : public QObject
{
public:
    ~DeviceDisconnectMonitor();
    std::optional<QFuture<void>> setDisconnectListener(AudioObjectID);

private:
    struct State
    {
        AudioObjectID id;
        std::shared_ptr<AudioObjectPropertyListenerBlock> block;
    };

    std::optional<State> m_state;
};
#endif

} // namespace QCoreAudioUtils

QT_END_NAMESPACE

#endif // QCOREAUDIODEVICES_H
