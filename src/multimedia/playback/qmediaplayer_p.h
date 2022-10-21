/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMEDIAPLAYER_P_H
#define QMEDIAPLAYER_P_H

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

#include "qmediaplayer.h"
#include "qmediametadata.h"
#include "qvideosink.h"
#include "qaudiooutput.h"
#include <private/qplatformmediaplayer_p.h>

#include "private/qobject_p.h"
#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qurl.h>
#include <QtCore/qfile.h>
#include <QtCore/qtimer.h>
QT_BEGIN_NAMESPACE

class QPlatformMediaPlayer;

class QMediaPlayerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QMediaPlayer)

public:
    QMediaPlayerPrivate() = default;
    QPlatformMediaPlayer* control = nullptr;
    QString errorString;

    QAudioOutput *audioOutput = nullptr;
    QVideoSink *videoSink = nullptr;
    QPointer<QObject> videoOutput;
    QUrl qrcMedia;
    QScopedPointer<QFile> qrcFile;
    QUrl source;
    QIODevice *stream = nullptr;

    QMediaPlayer::PlaybackState state = QMediaPlayer::StoppedState;
    QMediaPlayer::Error error = QMediaPlayer::NoError;

    void setMedia(const QUrl &media, QIODevice *stream = nullptr);

    QList<QMediaMetaData> trackMetaData(QPlatformMediaPlayer::TrackType s) const;

    void setState(QMediaPlayer::PlaybackState state);
    void setStatus(QMediaPlayer::MediaStatus status);
    void setError(int error, const QString &errorString);

    void setVideoSink(QVideoSink *sink)
    {
        Q_Q(QMediaPlayer);
        if (sink == videoSink)
            return;
        if (videoSink)
            videoSink->setSource(nullptr);
        videoSink = sink;
        if (sink)
            sink->setSource(q);
        control->setVideoSink(sink);
        emit q->videoOutputChanged();
    }
};

QT_END_NAMESPACE

#endif
