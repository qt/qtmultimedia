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

#include "qgstreamerformatsinfo_p.h"

#include "private/qgstutils_p.h"
#include "private/qgstcodecsinfo_p.h"

QT_BEGIN_NAMESPACE

static struct {
    const char *name;
    QMediaFormat::FileFormat value;
} videoFormatsMap[] = {
    { "video/x-ms-asf", QMediaFormat::FileFormat::ASF },
    { "video/x-msvideo", QMediaFormat::FileFormat::AVI },
    { "video/x-matroska", QMediaFormat::FileFormat::Matroska },
    { "video/quicktime, variant=(string)iso", QMediaFormat::FileFormat::MPEG4 },
    { "video/quicktime", QMediaFormat::FileFormat::QuickTime },
    { "video/ogg", QMediaFormat::FileFormat::Ogg },
    { "video/webm", QMediaFormat::FileFormat::WebM },
    { nullptr, QMediaFormat::FileFormat::MPEG4 }
};

static struct {
    const char *name;
    QMediaFormat::FileFormat value;
} audioFormatsMap[] = {
    { "audio/mpeg, mpegversion=(int)1, layer=(int)3", QMediaFormat::FileFormat::MP3 },
    { "audio/mpeg, mpegversion=(int)4", QMediaFormat::FileFormat::AAC },
    { "audio/x-flac", QMediaFormat::FileFormat::FLAC },
    { "audio/x-wma, wmaversion=(int)1", QMediaFormat::FileFormat::WindowsMediaAudio },
    { "audio/x-wma, wmaversion=(int)2", QMediaFormat::FileFormat::WindowsMediaAudio },
    { nullptr, QMediaFormat::FileFormat::MPEG4 },
};

static struct {
    const char *name;
    QMediaFormat::VideoCodec value;
} videoCodecMap[] = {
    { "video/mpeg, mpegversion=(int)1, systemstream=(boolean)false", QMediaFormat::VideoCodec::MPEG1 },
    { "video/mpeg, mpegversion=(int)2, systemstream=(boolean)false", QMediaFormat::VideoCodec::MPEG2 },
    { "video/mpeg, mpegversion=(int)4, systemstream=(boolean)false", QMediaFormat::VideoCodec::MPEG4 },
    { "video/x-h264", QMediaFormat::VideoCodec::H264 },
    { "video/x-h265", QMediaFormat::VideoCodec::H265 },
    { "video/x-vp8", QMediaFormat::VideoCodec::VP8 },
    { "video/x-vp9", QMediaFormat::VideoCodec::VP9 },
    { "video/x-av1", QMediaFormat::VideoCodec::AV1 },
    { "video/x-theora", QMediaFormat::VideoCodec::Theora },
    { "video/", QMediaFormat::VideoCodec::MotionJPEG },
    { nullptr, QMediaFormat::VideoCodec::Invalid }
};

static struct {
    const char *name;
    QMediaFormat::AudioCodec value;
} audioCodecMap[] = {
    { "audio/mpeg, mpegversion=(int)1, layer=(int)3", QMediaFormat::AudioCodec::MP3 },
    { "audio/mpeg, mpegversion=(int)4", QMediaFormat::AudioCodec::AAC },
    { "audio/x-ac3", QMediaFormat::AudioCodec::AC3 },
    { "audio/x-eac3", QMediaFormat::AudioCodec::EAC3 },
    { "audio/x-flac", QMediaFormat::AudioCodec::FLAC },
    { "audio/x-wma, wmaversion=(int)1", QMediaFormat::AudioCodec::WindowsMediaAudio },
    { "audio/x-wma, wmaversion=(int)2", QMediaFormat::AudioCodec::WindowsMediaAudio },
    { "audio/x-true-hd", QMediaFormat::AudioCodec::DolbyTrueHD },
    { "audio/x-vorbis", QMediaFormat::AudioCodec::Vorbis },
    { nullptr, QMediaFormat::AudioCodec::Invalid },
};

template<typename Map>
static auto getList(QGstCodecsInfo::ElementType type, Map *map)
{
    using T = decltype(map->value);
    QList<T> list;
    QGstCodecsInfo info(type);
    auto codecs = info.supportedCodecs();
    for (const auto &c : codecs) {
        Map *m = map;
        while (m->name) {
            if (m->name == c.toLatin1()) {
                list.append(m->value);
                break;
            }
            ++m;
        }
    }
    return list;
}

QGstreamerFormatsInfo::QGstreamerFormatsInfo()
{
    m_decodableMediaContainers = getList(QGstCodecsInfo::Demuxer, videoFormatsMap);
    m_decodableMediaContainers.append(getList(QGstCodecsInfo::AudioDecoder, audioFormatsMap));
    m_decodableAudioCodecs = getList(QGstCodecsInfo::AudioDecoder, audioCodecMap);
    m_decodableVideoCodecs = getList(QGstCodecsInfo::VideoDecoder, videoCodecMap);

    m_encodableMediaContainers = getList(QGstCodecsInfo::Muxer, videoFormatsMap);
    m_encodableMediaContainers.append(getList(QGstCodecsInfo::AudioEncoder, audioFormatsMap));
    m_encodableAudioCodecs = getList(QGstCodecsInfo::AudioEncoder, audioCodecMap);
    m_encodableVideoCodecs = getList(QGstCodecsInfo::VideoEncoder, videoCodecMap);
}

QGstreamerFormatsInfo::~QGstreamerFormatsInfo()
{

}


QList<QMediaFormat::FileFormat> QGstreamerFormatsInfo::decodableMediaContainers() const
{
    return m_decodableMediaContainers;
}

QList<QMediaFormat::AudioCodec> QGstreamerFormatsInfo::decodableAudioCodecs() const
{
    return m_decodableAudioCodecs;
}

QList<QMediaFormat::VideoCodec> QGstreamerFormatsInfo::decodableVideoCodecs() const
{
    return m_decodableVideoCodecs;
}

QList<QMediaFormat::FileFormat> QGstreamerFormatsInfo::encodableMediaContainers() const
{
    return m_encodableMediaContainers;
}

QList<QMediaFormat::AudioCodec> QGstreamerFormatsInfo::encodableAudioCodecs() const
{
    return m_encodableAudioCodecs;
}

QList<QMediaFormat::VideoCodec> QGstreamerFormatsInfo::encodableVideoCodecs() const
{
    return m_encodableVideoCodecs;
}

QT_END_NAMESPACE
