// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEG_P_H
#define QFFMPEG_P_H

#include <private/qtmultimediaglobal_p.h>
#include <qstring.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

#define QT_FFMPEG_OLD_CHANNEL_LAYOUT (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59,24,100))

QT_BEGIN_NAMESPACE

namespace QFFmpeg
{

inline qint64 timeStamp(qint64 ts, AVRational base)
{
    return (1000*ts*base.num + 500)/base.den;
}

inline qint64 timeStampUs(qint64 ts, AVRational base)
{
    return (1000000*ts*base.num + 500000)/base.den;
}

inline float toFloat(AVRational r) { return float(r.num)/float(r.den); }

inline QString err2str(int errnum)
{
    char buffer[AV_ERROR_MAX_STRING_SIZE + 1] = {};
    av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, errnum);
    return QString::fromLocal8Bit(buffer);
}

QT_END_NAMESPACE

}

#endif
