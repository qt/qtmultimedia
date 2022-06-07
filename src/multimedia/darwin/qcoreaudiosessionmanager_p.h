// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef IOSAUDIOSESSIONMANAGER_H
#define IOSAUDIOSESSIONMANAGER_H

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

#include <QObject>
#ifdef QT_DEBUG_COREAUDIO
# include <QtCore/QDebug>
#endif

@class CoreAudioSessionObserver;

QT_BEGIN_NAMESPACE

class CoreAudioSessionManager : public QObject
{
    Q_OBJECT
public:
    enum AudioSessionCategorys {
        Ambient,
        SoloAmbient,
        Playback,
        Record,
        PlayAndRecord,
        AudioProcessing,
        MultiRoute
    };
    enum AudioSessionCategoryOptions {
        None = 0,
        MixWithOthers = 1,
        DuckOthers = 2,
        AllowBluetooth = 4,
        DefaultToSpeaker = 8
    };
    enum AudioSessionModes {
        Default,
        VoiceChat,
        GameChat,
        VideoRecording,
        Measurement,
        MoviePlayback
    };

    static CoreAudioSessionManager& instance();

    bool setActive(bool active);
    bool setCategory(AudioSessionCategorys category, AudioSessionCategoryOptions options = None);
    bool setMode(AudioSessionModes mode);

    AudioSessionCategorys category();
    AudioSessionModes mode();

    QList<QByteArray> inputDevices();
    QList<QByteArray> outputDevices();

    float currentIOBufferDuration();
    float preferredSampleRate();

signals:
    void activeChanged();
    void categoryChanged();
    void modeChanged();
    void routeChanged();
    void devicesAvailableChanged();

private:
    CoreAudioSessionManager();
    ~CoreAudioSessionManager();
    CoreAudioSessionManager(CoreAudioSessionManager const &copy);
    CoreAudioSessionManager& operator =(CoreAudioSessionManager const &copy);

    CoreAudioSessionObserver *m_sessionObserver;
};

#ifdef QT_DEBUG_COREAUDIO
QDebug operator <<(QDebug dbg, CoreAudioSessionManager::AudioSessionCategorys category);
QDebug operator <<(QDebug dbg, CoreAudioSessionManager::AudioSessionCategoryOptions option);
QDebug operator <<(QDebug dbg, CoreAudioSessionManager::AudioSessionModes mode);
#endif

QT_END_NAMESPACE

#endif // IOSAUDIOSESSIONMANAGER_H
