// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAVAUDIOSESSIONMANAGER_P_H
#define QAVAUDIOSESSIONMANAGER_P_H

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

#include <QtMultimedia/qtmultimediaexports.h>

#include <QtCore/qobject.h>
#include <QtCore/qmutex.h>
#include <QtCore/private/qcore_mac_p.h>

#include <array>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#import <AVFoundation/AVAudioSession.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

struct SessionRequirement
{
    bool needsRecord = false;
    AVAudioSessionCategoryOptions options = 0;
};

class Q_MULTIMEDIA_EXPORT QAVAudioSessionManager final : public QObject
{
    Q_OBJECT
public:
    static QAVAudioSessionManager *instance();

    QAVAudioSessionManager();
    ~QAVAudioSessionManager() override;

    enum class TokenId : std::uint64_t {};

    class Q_MULTIMEDIA_EXPORT ActivationToken
    {
    public:
        ActivationToken() = default;
        ActivationToken(ActivationToken &&other) noexcept;
        ActivationToken &operator=(ActivationToken &&other) noexcept;
        ~ActivationToken();

        Q_DISABLE_COPY(ActivationToken)

    private:
        friend class QAVAudioSessionManager;
        ActivationToken(QAVAudioSessionManager *manager, std::shared_ptr<bool> destroyed,
                        TokenId tokenId);

        void release();

        QAVAudioSessionManager *m_manager = nullptr;
        std::shared_ptr<bool> m_destroyed;
        TokenId m_tokenId{};
    };

    ActivationToken activate(SessionRequirement requirement);

    // Reasserts activation of the shared AVAudioSession using the configuration derived from
    // currently live ActivationTokens, without changing which tokens are live. Intended for use
    // by QAVAudioSessionRecovery only, after the OS itself deactivated the session (e.g. an
    // interruption or media services reset) while our own set of clients hasn't changed.
    bool reassertActivation();

Q_SIGNALS:
    void interruptionBegan();
    void interruptionEnded(bool shouldResume);
    void mediaServicesWereLost();
    void mediaServicesWereReset();

private:
    friend class ActivationToken;

    void releaseActivation(TokenId tokenId);
    bool applyRequirementsLocked();

    QMutex m_activationMutex;
    std::vector<std::pair<TokenId, SessionRequirement>> m_requirements;
    std::uint64_t m_tokenIdAllocator = 0;

    std::array<QMacNotificationObserver, 3> m_observers;

    const std::shared_ptr<bool> m_destroyed = std::make_shared<bool>(false);
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QAVAUDIOSESSIONMANAGER_P_H
