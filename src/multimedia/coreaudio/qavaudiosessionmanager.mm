// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/private/qavaudiosessionmanager_p.h>

#include <QtCore/qapplicationstatic.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qthread.h>

#import <AVFoundation/AVAudioSession.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcAudioSession, "qt.multimedia.darwin.audiosession");

namespace QtMultimediaPrivate {

struct QAVAudioSessionManager::SessionConfiguration
{
    static SessionConfiguration fromSession()
    {
        return fromSession([AVAudioSession sharedInstance]);
    }

    static SessionConfiguration fromSession(AVAudioSession *session)
    {
        return SessionConfiguration{ session.category, session.categoryOptions, session.mode };
    }

    AVAudioSessionCategory category;
    AVAudioSessionCategoryOptions options;
    AVAudioSessionMode mode;

    friend bool operator==(const SessionConfiguration &lhs, const SessionConfiguration &rhs)
    {
        return [lhs.category isEqualToString:rhs.category] && lhs.options == rhs.options &&
                [lhs.mode isEqualToString:rhs.mode];
    }
    friend bool operator!=(const SessionConfiguration &lhs, const SessionConfiguration &rhs)
    {
        return !(lhs == rhs);
    }
};

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

} // namespace

Q_APPLICATION_STATIC(QAVAudioSessionManager, s_avAudioSessionManager)

QAVAudioSessionManager *QAVAudioSessionManager::instance()
{
    return s_avAudioSessionManager;
}

QAVAudioSessionManager::QAVAudioSessionManager()
    : m_configuration{ std::make_unique<SessionConfiguration>(SessionConfiguration::fromSession()) }
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

void QAVAudioSessionManager::updateConfiguration()
{
    *m_configuration = SessionConfiguration::fromSession();
}

bool QAVAudioSessionManager::activateSession()
{
    AVAudioSession *session = [AVAudioSession sharedInstance];
    SessionConfiguration currentConfig = SessionConfiguration::fromSession(session);

    // The audio session is process-wide and the application owns the category policy: e.g.
    // avfmediaplayer.mm calls updateConfiguration() after it sets a category for media
    // playback. We never choose a category of our own; we only ever restore the last
    // configuration the application itself put in place, so we don't fight app-driven
    // category changes or change ring/silent-switch behavior for existing iOS Qt apps.
    if (currentConfig != *m_configuration) {
        NSError *error = nil;
        if (![session setCategory:m_configuration->category
                      withOptions:m_configuration->options
                            error:&error]) {
            qCWarning(qLcAudioSession) << "failed to restore audio session category:"
                                       << QString::fromNSString(error.localizedDescription);
            return false;
        }
        if (![session setMode:m_configuration->mode error:&error]) {
            qCWarning(qLcAudioSession) << "failed to restore audio session mode:"
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

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#include "moc_qavaudiosessionmanager_p.cpp"
