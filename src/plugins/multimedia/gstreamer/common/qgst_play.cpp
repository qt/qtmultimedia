// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <common/qgst_play_p.h>

QT_BEGIN_NAMESPACE

namespace QGstPlaySupport {

VideoInfo parseGstPlayVideoInfo(const GstPlayVideoInfo *info)
{
    VideoInfo videoInfo;

    videoInfo.bitrate = gst_play_video_info_get_bitrate(info);
    videoInfo.maxBitrate = gst_play_video_info_get_max_bitrate(info);

    videoInfo.size = QSize{
        gst_play_video_info_get_width(info),
        gst_play_video_info_get_height(info),
    };

    {
        gint nom, denom;
        gst_play_video_info_get_framerate(info, &nom, &denom);
        videoInfo.framerate = Fraction{
            nom,
            denom,
        };
    }

    {
        guint nom, denom;
        gst_play_video_info_get_pixel_aspect_ratio(info, &nom, &denom);
        videoInfo.pixelAspectRatio = Fraction{
            int(nom),
            int(denom),
        };
    }

    return videoInfo;
}

AudioInfo parseGstPlayAudioInfo(const GstPlayAudioInfo *info)
{
    AudioInfo audioInfo;
    audioInfo.channels = gst_play_audio_info_get_channels(info);
    audioInfo.sampleRate = gst_play_audio_info_get_sample_rate(info);
    audioInfo.bitrate = gst_play_audio_info_get_bitrate(info);
    audioInfo.maxBitrate = gst_play_audio_info_get_max_bitrate(info);

    audioInfo.language = QLocale::codeToLanguage(
            QString::fromUtf8(gst_play_audio_info_get_language(info)), QLocale::AnyLanguageCode);

    return audioInfo;
}

SubtitleInfo parseGstPlaySubtitleInfo(const GstPlaySubtitleInfo *info)
{
    SubtitleInfo subtitleInfo;

    subtitleInfo.language = QLocale::codeToLanguage(
            QString::fromUtf8(gst_play_subtitle_info_get_language(info)), QLocale::AnyLanguageCode);

    return subtitleInfo;
}

int getStreamIndex(const GstPlayStreamInfo *info)
{
    if (info == nullptr)
        return -1;

    return gst_play_stream_info_get_index(info);
}

} // namespace QGstPlaySupport

QT_END_NAMESPACE
