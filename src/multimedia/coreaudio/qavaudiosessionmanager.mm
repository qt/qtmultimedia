// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qavaudiosessionmanager_p.h"

#include <QtMultimedia/private/qmultimedia_ranges_p.h>

#include <QtCore/qapplicationstatic.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qthread.h>

#import <AVFoundation/AVAudioSession.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcAudioSession, "qt.multimedia.darwin.audiosession");

namespace QtMultimediaPrivate {

using TokenId = QAVAudioSessionManager::TokenId;

namespace {

struct InterruptionInfo
{
    bool began;
    bool shouldResume;
};

InterruptionInfo parseInterruption(NSNotification *notification)
{
    NSDictionary *userInfo = notification.userInfo;
    NSNumber *typeValue = userInfo[AVAudioSessionInterruptionTypeKey];
    const auto type = static_cast<AVAudioSessionInterruptionType>(typeValue.unsignedIntegerValue);

    if (type == AVAudioSessionInterruptionTypeBegan) {
        qCDebug(qLcAudioSession) << "interruption notification: began";
        return { true, false };
    }

    // AVAudioSessionInterruptionOptionKey may be absent on "ended"; absence means "do not
    // resume", only AVAudioSessionInterruptionOptionShouldResume means resume.
    NSNumber *optionValue = userInfo[AVAudioSessionInterruptionOptionKey];
    const bool shouldResume = optionValue != nil
            && (optionValue.unsignedIntegerValue & AVAudioSessionInterruptionOptionShouldResume);
    qCDebug(qLcAudioSession) << "interruption notification: ended, shouldResume:" << shouldResume;
    return { false, shouldResume };
}

// Merges the requirements of all currently live ActivationTokens into a single category/options
// pair. The manager never chooses a category of its own beyond this merge: if any live client
// needs recording, the session must support it; options requested by any client (e.g. "mix with
// others") are unioned in.
AVAudioSessionCategory
mergedCategory(const std::vector<std::pair<TokenId, SessionRequirement>> &requirements)
{
    const bool needsRecord = ranges::any_of(requirements, [](const auto &entry) {
        return entry.second.needsRecord;
    });
    return needsRecord ? AVAudioSessionCategoryPlayAndRecord : AVAudioSessionCategoryPlayback;
}

AVAudioSessionCategoryOptions
mergedOptions(const std::vector<std::pair<TokenId, SessionRequirement>> &requirements)
{
    AVAudioSessionCategoryOptions options = 0;
    for (const auto &entry : requirements)
        options |= entry.second.options;
    return options;
}

} // namespace

Q_APPLICATION_STATIC(QAVAudioSessionManager, s_avAudioSessionManager)

QAVAudioSessionManager *QAVAudioSessionManager::instance()
{
    return s_avAudioSessionManager;
}

QAVAudioSessionManager::QAVAudioSessionManager()
{
    if (qApp && !QThread::isMainThread())
        moveToThread(qApp->thread());

    AVAudioSession *session = [AVAudioSession sharedInstance];
    const std::shared_ptr<bool> destroyed = m_destroyed;

    m_observers = {
        QMacNotificationObserver(session, AVAudioSessionInterruptionNotification,
                                 [this, destroyed](NSNotification *notification) {
        const InterruptionInfo info = parseInterruption(notification);
        QMetaObject::invokeMethod(this, [this, destroyed, info] {
            if (*destroyed)
                return;
            if (info.began)
                Q_EMIT interruptionBegan();
            else
                Q_EMIT interruptionEnded(info.shouldResume);
        });
    }),
        QMacNotificationObserver(session, AVAudioSessionMediaServicesWereLostNotification,
                                 [this, destroyed] {
        qCWarning(qLcAudioSession) << "media services were lost notification";
        QMetaObject::invokeMethod(this, [this, destroyed] {
            if (*destroyed)
                return;
            Q_EMIT mediaServicesWereLost();
        });
    }),
        QMacNotificationObserver(session, AVAudioSessionMediaServicesWereResetNotification,
                                 [this, destroyed] {
        qCWarning(qLcAudioSession) << "media services were reset notification";
        QMetaObject::invokeMethod(this, [this, destroyed] {
            if (*destroyed)
                return;
            Q_EMIT mediaServicesWereReset();
        });
    }),
    };
}

QAVAudioSessionManager::~QAVAudioSessionManager()
{
    m_observers = {};
    *m_destroyed = true;
}

// Must be called with m_activationMutex held.
bool QAVAudioSessionManager::applyRequirementsLocked()
{
    AVAudioSession *session = [AVAudioSession sharedInstance];
    const AVAudioSessionCategory category = mergedCategory(m_requirements);
    const AVAudioSessionCategoryOptions options = mergedOptions(m_requirements);

    if (![session.category isEqualToString:category] || session.categoryOptions != options) {
        NSError *error = nil;
        if (![session setCategory:category withOptions:options error:&error]) {
            qCWarning(qLcAudioSession) << "failed to set audio session category:"
                                       << QString::fromNSString(error.localizedDescription);
            return false;
        }
    }

    NSError *error = nil;
    const BOOL success = [session setActive:YES error:&error];
    if (!success)
        qCWarning(qLcAudioSession) << "failed to activate audio session:"
                                   << QString::fromNSString(error.localizedDescription);
    else
        qCDebug(qLcAudioSession) << "audio session activated";

    return success;
}

QAVAudioSessionManager::ActivationToken
QAVAudioSessionManager::activate(SessionRequirement requirement)
{
    const std::lock_guard guard(m_activationMutex);
    const TokenId tokenId{ ++m_tokenIdAllocator };
    m_requirements.emplace_back(tokenId, requirement);
    applyRequirementsLocked();
    return ActivationToken(this, m_destroyed, tokenId);
}

void QAVAudioSessionManager::releaseActivation(TokenId tokenId)
{
    const std::lock_guard guard(m_activationMutex);
    const auto it = ranges::find_if(m_requirements, [&](const auto &entry) {
        return entry.first == tokenId;
    });
    if (it != m_requirements.end())
        m_requirements.erase(it);

    if (m_requirements.empty()) {
        NSError *error = nil;
        if (![[AVAudioSession sharedInstance] setActive:NO error:&error])
            qCWarning(qLcAudioSession) << "failed to deactivate audio session:"
                                       << QString::fromNSString(error.localizedDescription);
        else
            qCDebug(qLcAudioSession) << "audio session deactivated";
        return;
    }

    applyRequirementsLocked();
}

bool QAVAudioSessionManager::reassertActivation()
{
    const std::lock_guard guard(m_activationMutex);
    if (m_requirements.empty()) {
        NSError *error = nil;
        const BOOL success = [[AVAudioSession sharedInstance] setActive:YES error:&error];
        if (!success)
            qCWarning(qLcAudioSession) << "failed to activate audio session:"
                                       << QString::fromNSString(error.localizedDescription);
        return success;
    }

    return applyRequirementsLocked();
}

QAVAudioSessionManager::ActivationToken::ActivationToken(QAVAudioSessionManager *manager,
                                                         std::shared_ptr<bool> destroyed,
                                                         TokenId tokenId)
    : m_manager(manager), m_destroyed(std::move(destroyed)), m_tokenId(tokenId)
{
}

QAVAudioSessionManager::ActivationToken::ActivationToken(ActivationToken &&other) noexcept
    : m_manager(std::exchange(other.m_manager, nullptr)),
      m_destroyed(std::move(other.m_destroyed)),
      m_tokenId(other.m_tokenId)
{
}

QAVAudioSessionManager::ActivationToken &
QAVAudioSessionManager::ActivationToken::operator=(ActivationToken &&other) noexcept
{
    if (this != &other) {
        release();
        m_manager = std::exchange(other.m_manager, nullptr);
        m_destroyed = std::move(other.m_destroyed);
        m_tokenId = other.m_tokenId;
    }
    return *this;
}

QAVAudioSessionManager::ActivationToken::~ActivationToken()
{
    release();
}

void QAVAudioSessionManager::ActivationToken::release()
{
    if (m_manager && m_destroyed && !*m_destroyed)
        m_manager->releaseActivation(m_tokenId);
    m_manager = nullptr;
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#include "moc_qavaudiosessionmanager_p.cpp"
