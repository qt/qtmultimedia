/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
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
****************************************************************************/

#ifndef QWINRTMEDIAPLAYERCONTROL_H
#define QWINRTMEDIAPLAYERCONTROL_H

#include <QtMultimedia/QMediaPlayerControl>

struct IMFMediaEngineClassFactory;

QT_BEGIN_NAMESPACE

class QVideoRendererControl;

class QWinRTMediaPlayerControlPrivate;
class QWinRTMediaPlayerControl : public QMediaPlayerControl
{
    Q_OBJECT
public:
    QWinRTMediaPlayerControl(IMFMediaEngineClassFactory *factory, QObject *parent = nullptr);
    ~QWinRTMediaPlayerControl() override = default;

    QMediaPlayer::State state() const override;
    QMediaPlayer::MediaStatus mediaStatus() const override;

    qint64 duration() const override;

    qint64 position() const override;
    void setPosition(qint64 position) override;

    int volume() const override;
    void setVolume(int volume) override;

    bool isMuted() const override;
    void setMuted(bool muted) override;

    int bufferStatus() const override;

    bool isAudioAvailable() const override;
    bool isVideoAvailable() const override;

    bool isSeekable() const override;

    QMediaTimeRange availablePlaybackRanges() const override;

    qreal playbackRate() const override;
    void setPlaybackRate(qreal rate) override;

    QMediaContent media() const override;
    const QIODevice *mediaStream() const override;
    void setMedia(const QMediaContent &media, QIODevice *stream) override;

    void play() override;
    void pause() override;
    void stop() override;

    QVideoRendererControl *videoRendererControl();

private:
    QScopedPointer<QWinRTMediaPlayerControlPrivate, QWinRTMediaPlayerControlPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTMediaPlayerControl)
};

QT_END_NAMESPACE

#endif // QWINRTMEDIAPLAYERCONTROL_H
