// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGST_PLAY_P_H
#define QGST_PLAY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qlocale.h>

#include <QtMultimedia/private/qmultimediautils_p.h>

#include "common/qgst_handle_types_p.h"

#include <gst/play/gstplay.h>

QT_BEGIN_NAMESPACE

namespace QGstPlaySupport {

using QUniqueGstPlayMediaInfoHandle =
        QGstImpl::QGObjectHandleHelper<GstPlayMediaInfo>::SharedHandle;
using QUniqueGstPlayAudioInfoHandle =
        QGstImpl::QGObjectHandleHelper<GstPlayAudioInfo>::SharedHandle;
using QUniqueGstPlayVideoInfoHandle =
        QGstImpl::QGObjectHandleHelper<GstPlayVideoInfo>::SharedHandle;
using QUniqueGstPlaySubtitleInfoHandle =
        QGstImpl::QGObjectHandleHelper<GstPlaySubtitleInfo>::SharedHandle;

struct VideoInfo
{
    int bitrate{};
    int maxBitrate{};
    QSize size;
    Fraction framerate{};
    Fraction pixelAspectRatio{};
};

VideoInfo parseGstPlayVideoInfo(const GstPlayVideoInfo *);

struct AudioInfo
{
    int channels{};
    int sampleRate{};
    int bitrate{};
    int maxBitrate{};
    QLocale::Language language{};
};

AudioInfo parseGstPlayAudioInfo(const GstPlayAudioInfo *);

struct SubtitleInfo
{
    QLocale::Language language{};
};

SubtitleInfo parseGstPlaySubtitleInfo(const GstPlaySubtitleInfo *);

template <typename T>
constexpr bool isStreamType = std::is_same_v<std::decay_t<T>, GstPlayVideoInfo>
        || std::is_same_v<std::decay_t<T>, GstPlayAudioInfo>
        || std::is_same_v<std::decay_t<T>, GstPlaySubtitleInfo>;

int getStreamIndex(const GstPlayStreamInfo *);

template <typename T, typename Enabler = std::enable_if_t<isStreamType<T>>>
int getStreamIndex(const T *info)
{
    return getStreamIndex(GST_PLAY_STREAM_INFO(info));
}

} // namespace QGstPlaySupport

QT_END_NAMESPACE

#endif
