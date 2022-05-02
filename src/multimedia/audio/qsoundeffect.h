/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QSOUNDEFFECT_H
#define QSOUNDEFFECT_H

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qaudio.h>
#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qstringlist.h>


QT_BEGIN_NAMESPACE


class QSoundEffectPrivate;
class QAudioDevice;

class Q_MULTIMEDIA_EXPORT QSoundEffect : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("DefaultMethod", "play()")
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int loops READ loopCount WRITE setLoopCount NOTIFY loopCountChanged)
    Q_PROPERTY(int loopsRemaining READ loopsRemaining NOTIFY loopsRemainingChanged)
    Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QAudioDevice audioDevice READ audioDevice WRITE setAudioDevice NOTIFY audioDeviceChanged)

public:
    enum Loop
    {
        Infinite = -2
    };
    Q_ENUM(Loop)

    enum Status
    {
        Null,
        Loading,
        Ready,
        Error
    };
    Q_ENUM(Status)

    explicit QSoundEffect(QObject *parent = nullptr);
    explicit QSoundEffect(const QAudioDevice &audioDevice, QObject *parent = nullptr);
    ~QSoundEffect();

    static QStringList supportedMimeTypes();

    QUrl source() const;
    void setSource(const QUrl &url);

    int loopCount() const;
    int loopsRemaining() const;
    void setLoopCount(int loopCount);

    QAudioDevice audioDevice();
    void setAudioDevice(const QAudioDevice &device);

    float volume() const;
    void setVolume(float volume);

    bool isMuted() const;
    void setMuted(bool muted);

    bool isLoaded() const;

    bool isPlaying() const;
    Status status() const;

Q_SIGNALS:
    void sourceChanged();
    void loopCountChanged();
    void loopsRemainingChanged();
    void volumeChanged();
    void mutedChanged();
    void loadedChanged();
    void playingChanged();
    void statusChanged();
    void audioDeviceChanged();

public Q_SLOTS:
    void play();
    void stop();

private:
    Q_DISABLE_COPY(QSoundEffect)
    QSoundEffectPrivate *d = nullptr;
};

QT_END_NAMESPACE


#endif // QSOUNDEFFECT_H
