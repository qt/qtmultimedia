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
#include <QtCore/private/qcore_mac_p.h>

#include <array>
#include <memory>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

class Q_MULTIMEDIA_EXPORT QAVAudioSessionManager final : public QObject
{
    Q_OBJECT
public:
    static QAVAudioSessionManager *instance();

    QAVAudioSessionManager();
    ~QAVAudioSessionManager() override;

    bool activateSession();

    // Must be called by any code that intentionally changes the shared AVAudioSession's
    // category, options or mode (e.g. avfmediaplayer.mm), so that activateSession() treats
    // this as the configuration to restore after an interruption or media services reset,
    // instead of reverting to whatever was in effect when the monitor was constructed.
    void updateConfiguration();

Q_SIGNALS:
    void interruptionBegan();
    void interruptionEnded(bool shouldResume);
    void mediaServicesWereLost();
    void mediaServicesWereReset();

private:
    struct SessionConfiguration;

    const std::unique_ptr<SessionConfiguration> m_configuration;
    std::array<QMacNotificationObserver, 3> m_observers;

    const std::shared_ptr<bool> m_destroyed = std::make_shared<bool>(false);
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QAVAUDIOSESSIONMANAGER_P_H
