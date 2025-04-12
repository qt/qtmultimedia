// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrtaudioengine_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>

#include <QtMultimedia/private/qaudio_rtsan_support_p.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qmemory_resource_tlsf_p.h>

#include <QtCore/q20map.h>
#include <mutex>

#ifdef Q_CC_MINGW
// mingw-13.1 seems to have a false positive when using std::function inside a std::variant
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wmaybe-uninitialized")
#endif

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

using namespace QtPrivate;
using namespace std::chrono_literals;

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace {
struct AudioDeviceFormatLess
{
    bool operator()(const std::pair<QAudioDevice, QAudioFormat> &lhs,
                    const std::pair<QAudioDevice, QAudioFormat> &rhs) const
    {
        auto cmp = qCompareThreeWay(lhs.first.id(), rhs.first.id());
        if (cmp == Qt::strong_ordering::less)
            return true;
        if (cmp == Qt::strong_ordering::greater)
            return false;

        return std::tuple(lhs.second.sampleRate(), lhs.second.sampleFormat(),
                          lhs.second.channelCount())
                < std::tuple(rhs.second.sampleRate(), rhs.second.sampleFormat(),
                             rhs.second.channelCount());
    }
};
} // namespace

std::shared_ptr<QRtAudioEngine>
QRtAudioEngine::getEngineFor(const QAudioDevice &device, const QAudioFormat &format)
{
    if (device.isNull()) {
        qWarning() << "QRtAudioEngine needs to be called with a valid device";
        return nullptr;
    }

    if (format.sampleFormat() != QAudioFormat::Float) {
        qWarning() << "QRtAudioEngine requires floating point samples";
        return nullptr;
    }

    if (!device.isFormatSupported(format)) {
        qWarning() << "QRtAudioEngine needs to be called with a supported fromat";
        return nullptr;
    }

    static QMutex s_playerRegistryMutex;
    static std::map<std::pair<QAudioDevice, QAudioFormat>, std::weak_ptr<QRtAudioEngine>,
                    AudioDeviceFormatLess>
            s_playerRegistry;

    auto guard = std::lock_guard{ s_playerRegistryMutex };

    auto key = std::pair{ device, format };
    auto found = s_playerRegistry.find(key);
    if (found != s_playerRegistry.end()) {
        auto player = found->second.lock();
        if (player)
            return player;
    }

    // lazy clean up
    q20::erase_if(s_playerRegistry, [](auto &&keyValuePair) {
        return keyValuePair.second.expired();
    });

    auto player = std::shared_ptr<QRtAudioEngine>(new QRtAudioEngine{ device, format },
                                                  [](QRtAudioEngine *engine) {
        engine->deleteLater();
    });
    s_playerRegistry.emplace(key, player);

    return player;
}

QRtAudioEngine::QRtAudioEngine(const QAudioDevice &device, const QAudioFormat &format)
    : m_sink{
          device,
          format,
      },
      m_rtMemoryPool {
           std::make_unique<QTlsfMemoryResource>(poolSize)
      }
{
    m_notificationEvent.callOnActivated([this] {
        runNonRtNotifications();
    });

    if (!QThread::isMainThread()) {
        QThread *appThread = qApp->thread();
        moveToThread(appThread);
        m_sink.moveToThread(appThread);
        m_notificationEvent.moveToThread(appThread);
        m_pendingCommandsTimer.moveToThread(appThread);
    }

    m_pendingCommandsTimer.setInterval(10ms);
    m_pendingCommandsTimer.setTimerType(Qt::CoarseTimer);
    m_pendingCommandsTimer.callOnTimeout(&m_pendingCommandsTimer, [this] {
        auto lock = std::lock_guard{ m_mutex };
        sendPendingRtCommands();
        if (m_appToRtOverflowBuffer.empty())
            m_pendingCommandsTimer.stop();
    });

    QPlatformAudioSink *platformSink = QPlatformAudioSink::get(m_sink);

    platformSink->start([this](QSpan<float> outputBuffer) {
        audioCallback(outputBuffer);
    });

    // we start suspended
    platformSink->suspend();
}

QRtAudioEngine::~QRtAudioEngine()
{
    m_sink.reset();

    // consume the ringbuffers
    m_appToRt.consumeAll([](auto) {
    });
    m_rtToApp.consumeAll([](auto) {
    });
}

void QRtAudioEngine::play(SharedVoice voice)
{
    auto lock = std::lock_guard{ m_mutex };

    // TODO: where do we expect reampling to happen?
    Q_ASSERT(voice->format() == m_sink.format());

    if (m_voices.empty())
        m_sink.resume();

    m_voices.insert(voice);

    sendAppToRtCommand(PlayCommand{
            std::move(voice),
    });
}

void QRtAudioEngine::stop(const SharedVoice &voice)
{
    stop(voice->voiceId());
}

void QRtAudioEngine::stop(VoiceId voiceId)
{
    auto lock = std::lock_guard{ m_mutex };
    sendAppToRtCommand(StopCommand{ voiceId });
}

void QRtAudioEngine::visitVoiceRt(VoiceId voiceId, RtVoiceVisitor fn, bool visitorIsTrivial)
{
    auto lock = std::lock_guard{ m_mutex };

    if (visitorIsTrivial) {
        sendAppToRtCommand(VisitCommandTrivial{
                voiceId,
                std::move(fn),
        });

    } else {
        sendAppToRtCommand(VisitCommand{
                voiceId,
                std::move(fn),
        });
    }
}

VoiceId QRtAudioEngine::allocateVoiceId()
{
    static std::atomic_uint64_t allocator{ 0 };
    return VoiceId{ allocator.fetch_add(1, std::memory_order_relaxed) };
}

void QRtAudioEngine::audioCallback(QSpan<float> outputBuffer) noexcept QT_MM_NONBLOCKING
{
    runRtCommands();
    bool sendNotification = sendPendingAppNotifications();

    std::fill(outputBuffer.begin(), outputBuffer.end(), 0.f);

    std::vector<SharedVoice, pmr::polymorphic_allocator<SharedVoice>> finishedVoices{
        m_rtMemoryPool.get(),
    };

    for (const SharedVoice &voice : m_rtVoiceRegistry) {
        Q_ASSERT(voice.use_count() >= 2); // voice in both m_rtVoiceRegistry and m_voices

        VoicePlayResult playResult = voice->play(outputBuffer);
        if (playResult == VoicePlayResult::Finished)
            finishedVoices.push_back(voice);
    }

    if (!finishedVoices.empty()) {
        withRTSanDisabled([&] {
            for (const SharedVoice &voice : finishedVoices) {
                m_rtVoiceRegistry.erase(voice);
                bool stopSent = sendRtToAppNotification(StopNotification{ voice });
                if (stopSent)
                    sendNotification = true;
            }
        });
    }

    // TODO: we should probably (soft)clip the output buffer

    cleanupRetiredVoices();
    if (sendNotification)
        m_notificationEvent.set();
}

void QRtAudioEngine::cleanupRetiredVoices() noexcept QT_MM_NONBLOCKING
{
    bool notifyApp = false;

#if __cpp_lib_erase_if >= 202002L
    using std::erase_if;
#else
    auto erase_if = [](auto &c, auto &&pred) {
        auto old_size = c.size();
        for (auto first = c.begin(), last = c.end(); first != last;) {
            if (pred(*first))
                first = c.erase(first);
            else
                ++first;
        }
        return old_size - c.size();
    };
#endif
    withRTSanDisabled([&] {
        erase_if(m_rtVoiceRegistry, [&](const SharedVoice &voice) {
            bool voiceIsActive = voice->isActive();
            if (!voiceIsActive)
                notifyApp = sendRtToAppNotification(StopNotification{ voice });

            return false;
        });
    });

    if (notifyApp)
        m_notificationEvent.set();
}

void QRtAudioEngine::runRtCommands() noexcept QT_MM_NONBLOCKING
{
    m_appToRt.consumeAll([&](QSpan<RtCommand> commands) {
        for (RtCommand &cmd : commands) {
            std::visit([&](auto cmd) {
                runRtCommand(std::move(cmd));
            }, std::move(cmd));
        }
    });
}

void QRtAudioEngine::runRtCommand(PlayCommand cmd) noexcept QT_MM_NONBLOCKING
{
    withRTSanDisabled([&] {
        m_rtVoiceRegistry.insert(cmd.voice);
    });
}

void QRtAudioEngine::runRtCommand(StopCommand cmd) noexcept QT_MM_NONBLOCKING
{
    auto it = m_rtVoiceRegistry.find(cmd.voiceId);
    Q_ASSERT(it != m_rtVoiceRegistry.end());

    SharedVoice voice = *it;
    m_rtVoiceRegistry.erase(it);

    bool emitNotify = sendRtToAppNotification(StopNotification{
            std::move(voice),
    });
    if (emitNotify)
        m_notificationEvent.set();
}

void QRtAudioEngine::runRtCommand(VisitCommand cmd) noexcept QT_MM_NONBLOCKING
{
    auto it = m_rtVoiceRegistry.find(cmd.voiceId);
    Q_ASSERT(it != m_rtVoiceRegistry.end());

    cmd.callback(**it);

    // send callback back to application for destruction
    bool emitNotify = sendRtToAppNotification(VisitReply{
            std::move(cmd.callback),
    });
    if (emitNotify)
        m_notificationEvent.set();
}

void QRtAudioEngine::runRtCommand(VisitCommandTrivial cmd) noexcept QT_MM_NONBLOCKING
{
    auto it = m_rtVoiceRegistry.find(cmd.voiceId);
    Q_ASSERT(it != m_rtVoiceRegistry.end());

    cmd.callback(**it);
}

void QRtAudioEngine::runNonRtNotifications()
{
    std::vector<VoiceId> finishedVoices;
    {
        auto lock = std::lock_guard{ m_mutex };
        m_rtToApp.consumeAll([&](QSpan<Notification> notifications) {
            for (Notification &notification : notifications) {
                std::visit([&](auto notification) {
                    runNonRtNotification(std::move(notification));
                }, std::move(notification));
            }
        });

        finishedVoices = std::move(m_finishedVoices);
        m_finishedVoices.clear();
    }

    // emit voiceFinished outside of the lock
    for (VoiceId voiceId : finishedVoices)
        emit voiceFinished(voiceId);
}

void QRtAudioEngine::runNonRtNotification(StopNotification notification)
{
    m_voices.erase(notification.voice);
    if (m_voices.empty())
        m_sink.suspend();
    m_finishedVoices.push_back(notification.voice->voiceId());
}

void QRtAudioEngine::runNonRtNotification(VisitReply)
{
    // nop (just making sure to delete on the application thread);
}

void QRtAudioEngine::sendAppToRtCommand(RtCommand cmd)
{
    // first write all pending commands from overflow buffer
    sendPendingRtCommands();

    bool written = m_appToRt.produceOne([&] {
        return std::move(cmd);
    });

    if (written)
        return;

    m_appToRtOverflowBuffer.emplace_back(std::move(cmd));

    QMetaObject::invokeMethod(&m_pendingCommandsTimer, [this] {
        if (!m_pendingCommandsTimer.isActive())
            m_pendingCommandsTimer.start();
    });
}

bool QRtAudioEngine::sendRtToAppNotification(Notification cmd)
{
    // first write all pending commands from overflow buffer
    bool emitNotification = sendPendingAppNotifications();

    bool written = m_rtToApp.produceOne([&] {
        return std::move(cmd);
    });

    if (written)
        return true;

    m_rtToAppOverflowBuffer.emplace_back(std::move(cmd));

    return emitNotification;
}

void QRtAudioEngine::sendPendingRtCommands()
{
    while (!m_appToRtOverflowBuffer.empty()) {
        Q_UNLIKELY_BRANCH;
        bool written = m_appToRt.produceOne([&] {
            return std::move(m_appToRtOverflowBuffer.front());
        });
        if (!written)
            return;

        m_appToRtOverflowBuffer.pop_front();
    }
}

bool QRtAudioEngine::sendPendingAppNotifications()
{
    bool emitNotification = false;
    while (!m_rtToAppOverflowBuffer.empty()) {
        Q_UNLIKELY_BRANCH;

        // first write all pending commands from overflow buffer
        bool written = m_rtToApp.produceOne([&] {
            return std::move(m_rtToAppOverflowBuffer.front());
        });
        if (!written)
            break;

        m_rtToAppOverflowBuffer.pop_front();
        emitNotification = true;
    }

    return emitNotification;
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#ifdef Q_CC_MINGW
QT_WARNING_POP
#endif

#include "moc_qrtaudioengine_p.cpp"
