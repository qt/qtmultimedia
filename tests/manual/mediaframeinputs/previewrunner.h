// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef PREVIEWRUNNER_H
#define PREVIEWRUNNER_H

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>

class PreviewRunner : public QObject
{
    Q_OBJECT
public:
    PreviewRunner();

    void run(const QUrl &mediaLocation);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void handlePlaybackStateChanged(QMediaPlayer::PlaybackState state);

    void handleError(QMediaPlayer::Error error, QString description);

    void updateTitle();

signals:
    void finished();

private:
    QVideoWidget m_preview;
    QAudioOutput m_audioOutput;
    QMediaPlayer m_player;
};

#endif // PREVIEWRUNNER_H
