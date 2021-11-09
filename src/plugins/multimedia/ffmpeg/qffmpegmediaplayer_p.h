/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QFFMPEGMEDIAPLAYER_H
#define QFFMPEGMEDIAPLAYER_H

#include <private/qplatformmediaplayer_p.h>
#include <qmediametadata.h>
#include "qffmpeg_p.h"

QT_BEGIN_NAMESPACE
class QFFmpegDecoder;
class QPlatformAudioOutput;

class QFFmpegMediaPlayer : public QPlatformMediaPlayer
{
public:
    QFFmpegMediaPlayer(QMediaPlayer *player);
    ~QFFmpegMediaPlayer();

    qint64 duration() const override;

    qint64 position() const override;
    void setPosition(qint64 position) override;

    float bufferProgress() const override;

    QMediaTimeRange availablePlaybackRanges() const override;

    qreal playbackRate() const override;
    void setPlaybackRate(qreal rate) override;

    QUrl media() const override;
    const QIODevice *mediaStream() const override;
    void setMedia(const QUrl &media, QIODevice *stream) override;

    void play() override;
    void pause() override;
    void stop() override;

//    bool streamPlaybackSupported() const { return false; }

    void setAudioOutput(QPlatformAudioOutput *) override;

    QMediaMetaData metaData() const override { return m_metaData; }

    void setVideoSink(QVideoSink *sink) override;
    QVideoSink *videoSink() const { return m_sink; }

    int trackCount(TrackType) override;
    QMediaMetaData trackMetaData(TrackType type, int streamNumber) override;
    int activeTrack(TrackType) override;
    void setActiveTrack(TrackType, int streamNumber) override;

private:
    friend class QFFmpegDecoder;
    friend class DecoderThread;

    QFFmpegDecoder *decoder;
    void closeContext();
    void checkStreams();
    void openCodec(TrackType type, int index);
    void closeCodec(TrackType type);

    struct StreamInfo {
        int avStreamIndex = -1;
        QMediaMetaData metaData;
    };

    QList<StreamInfo> m_streamMap[QPlatformMediaPlayer::NTrackTypes];

    AVFormatContext *context = nullptr;
    int m_currentStream[QPlatformMediaPlayer::NTrackTypes] = { -1, -1, -1 };
    AVCodecContext *codecContext[QPlatformMediaPlayer::NTrackTypes] = {};

    QUrl m_url;
    QIODevice *m_device = nullptr;
    QVideoSink *m_sink = nullptr;
    qint64 m_duration = 0;
    QMediaMetaData m_metaData;
    QPlatformAudioOutput *m_audioOutput = nullptr;
};

QT_END_NAMESPACE


#endif  // QMEDIAPLAYERCONTROL_H

