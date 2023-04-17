// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MEDIAFILESELECTOR_H
#define MEDIAFILESELECTOR_H

#include <QUrl>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <qsignalspy.h>
#include <qfileinfo.h>
#include <qtest.h>

QT_BEGIN_NAMESPACE

namespace MediaFileSelector {

// TODO: refactor or remove the function
inline QUrl selectMediaFile(const QStringList &mediaCandidates)
{
    QAudioOutput audioOutput;
    QVideoSink videoOutput;
    QMediaPlayer player;
    player.setAudioOutput(&audioOutput);
    player.setVideoOutput(&videoOutput);

    QSignalSpy errorSpy(&player, SIGNAL(errorOccurred(QMediaPlayer::Error, const QString&)));

    for (const QString &media : mediaCandidates) {
        player.setSource(media);
        player.play();

        for (int i = 0; i < 2000 && player.mediaStatus() != QMediaPlayer::BufferedMedia && errorSpy.isEmpty(); i+=50) {
            QTest::qWait(50);
        }

        if (player.mediaStatus() == QMediaPlayer::BufferedMedia && errorSpy.isEmpty()) {
            return media;
        }
        errorSpy.clear();
    }

    return QUrl();
}

} // MediaFileSelector namespace

QT_END_NAMESPACE

#endif

