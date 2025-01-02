// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamerformatinfo_p.h"

#include "qgstutils_p.h"

QT_BEGIN_NAMESPACE

QMediaFormat::AudioCodec QGstreamerFormatInfo::audioCodecForCaps(QGstStructure structure)
{
    const char *name = structure.name().data();

    if (!name || strncmp(name, "audio/", 6))
        return QMediaFormat::AudioCodec::Unspecified;
    name += 6;
    if (!strcmp(name, "mpeg")) {
        auto version = structure["mpegversion"].toInt();
        if (version == 1) {
            auto layer = structure["layer"];
            if (!layer.isNull())
                return QMediaFormat::AudioCodec::MP3;
        }
        if (version == 4)
            return QMediaFormat::AudioCodec::AAC;
    } else if (!strcmp(name, "x-ac3")) {
        return QMediaFormat::AudioCodec::AC3;
    } else if (!strcmp(name, "x-eac3")) {
        return QMediaFormat::AudioCodec::EAC3;
    } else if (!strcmp(name, "x-flac")) {
        return QMediaFormat::AudioCodec::FLAC;
    } else if (!strcmp(name, "x-alac")) {
        return QMediaFormat::AudioCodec::ALAC;
    } else if (!strcmp(name, "x-true-hd")) {
        return QMediaFormat::AudioCodec::DolbyTrueHD;
    } else if (!strcmp(name, "x-vorbis")) {
        return QMediaFormat::AudioCodec::Vorbis;
    } else if (!strcmp(name, "x-opus")) {
        return QMediaFormat::AudioCodec::Opus;
    } else if (!strcmp(name, "x-wav")) {
        return QMediaFormat::AudioCodec::Wave;
    } else if (!strcmp(name, "x-wma")) {
        return QMediaFormat::AudioCodec::WMA;
    }
    return QMediaFormat::AudioCodec::Unspecified;
}

QMediaFormat::VideoCodec QGstreamerFormatInfo::videoCodecForCaps(QGstStructure structure)
{
    const char *name = structure.name().data();

    if (!name || strncmp(name, "video/", 6))
        return QMediaFormat::VideoCodec::Unspecified;
    name += 6;

    if (!strcmp(name, "mpeg")) {
        auto version = structure["mpegversion"].toInt();
        if (version == 1)
            return QMediaFormat::VideoCodec::MPEG1;
        else if (version == 2)
            return QMediaFormat::VideoCodec::MPEG2;
        else if (version == 4)
            return QMediaFormat::VideoCodec::MPEG4;
    } else if (!strcmp(name, "x-h264")) {
        return QMediaFormat::VideoCodec::H264;
#if GST_CHECK_VERSION(1, 17, 0) // x265enc seems to be broken on 1.16 at least
    } else if (!strcmp(name, "x-h265")) {
        return QMediaFormat::VideoCodec::H265;
#endif
    } else if (!strcmp(name, "x-vp8")) {
        return QMediaFormat::VideoCodec::VP8;
    } else if (!strcmp(name, "x-vp9")) {
        return QMediaFormat::VideoCodec::VP9;
    } else if (!strcmp(name, "x-av1")) {
        return QMediaFormat::VideoCodec::AV1;
    } else if (!strcmp(name, "x-theora")) {
        return QMediaFormat::VideoCodec::Theora;
    } else if (!strcmp(name, "x-jpeg")) {
        return QMediaFormat::VideoCodec::MotionJPEG;
    } else if (!strcmp(name, "x-wmv")) {
        return QMediaFormat::VideoCodec::WMV;
    }
    return QMediaFormat::VideoCodec::Unspecified;
}

QMediaFormat::FileFormat QGstreamerFormatInfo::fileFormatForCaps(QGstStructure structure)
{
    const char *name = structure.name().data();

    if (!strcmp(name, "video/x-ms-asf")) {
        return QMediaFormat::FileFormat::WMV;
    } else if (!strcmp(name, "video/x-msvideo")) {
        return QMediaFormat::FileFormat::AVI;
    } else if (!strcmp(name, "video/x-matroska")) {
        return QMediaFormat::FileFormat::Matroska;
    } else if (!strcmp(name, "video/quicktime")) {
        auto variant = structure["variant"].toString();
        if (!variant)
            return QMediaFormat::FileFormat::QuickTime;
        else if (!strcmp(variant, "iso"))
            return QMediaFormat::FileFormat::MPEG4;
    } else if (!strcmp(name, "video/ogg")) {
        return QMediaFormat::FileFormat::Ogg;
    } else if (!strcmp(name, "video/webm")) {
        return QMediaFormat::FileFormat::WebM;
    } else if (!strcmp(name, "audio/x-m4a")) {
        return QMediaFormat::FileFormat::Mpeg4Audio;
    } else if (!strcmp(name, "audio/x-wav")) {
        return QMediaFormat::FileFormat::Wave;
    } else if (!strcmp(name, "audio/mpeg")) {
        auto mpegversion = structure["mpegversion"].toInt();
        if (mpegversion == 1) {
            auto layer = structure["layer"];
            if (!layer.isNull())
                return QMediaFormat::FileFormat::MP3;
        }
    }
    return QMediaFormat::UnspecifiedFormat;
}


QImageCapture::FileFormat QGstreamerFormatInfo::imageFormatForCaps(QGstStructure structure)
{
    const char *name = structure.name().data();

    if (!strcmp(name, "image/jpeg")) {
        return QImageCapture::JPEG;
    } else if (!strcmp(name, "image/png")) {
        return QImageCapture::PNG;
    } else if (!strcmp(name, "image/webp")) {
        return QImageCapture::WebP;
    } else if (!strcmp(name, "image/tiff")) {
        return QImageCapture::Tiff;
    }
    return QImageCapture::UnspecifiedFormat;
}

static QPair<QList<QMediaFormat::AudioCodec>, QList<QMediaFormat::VideoCodec>> getCodecsList(bool decode)
{
    QList<QMediaFormat::AudioCodec> audio;
    QList<QMediaFormat::VideoCodec> video;

    GstPadDirection padDirection = decode ? GST_PAD_SINK : GST_PAD_SRC;

    GList *elementList = gst_element_factory_list_get_elements(decode ? GST_ELEMENT_FACTORY_TYPE_DECODER : GST_ELEMENT_FACTORY_TYPE_ENCODER,
                                                               GST_RANK_MARGINAL);

    GList *element = elementList;
    while (element) {
        GstElementFactory *factory = (GstElementFactory *)element->data;
        element = element->next;

        const GList *padTemplates = gst_element_factory_get_static_pad_templates(factory);
        while (padTemplates) {
            GstStaticPadTemplate *padTemplate = (GstStaticPadTemplate *)padTemplates->data;
            padTemplates = padTemplates->next;

            if (padTemplate->direction == padDirection) {
                auto caps = QGstCaps(gst_static_caps_get(&padTemplate->static_caps), QGstCaps::HasRef);

                for (int i = 0; i < caps.size(); i++) {
                    QGstStructure structure = caps.at(i);
                    auto a = QGstreamerFormatInfo::audioCodecForCaps(structure);
                    if (a != QMediaFormat::AudioCodec::Unspecified && !audio.contains(a))
                        audio.append(a);
                    auto v = QGstreamerFormatInfo::videoCodecForCaps(structure);
                    if (v != QMediaFormat::VideoCodec::Unspecified && !video.contains(v))
                        video.append(v);
                }
            }
        }
    }
    gst_plugin_feature_list_free(elementList);
    return {audio, video};
}


QList<QGstreamerFormatInfo::CodecMap> QGstreamerFormatInfo::getMuxerList(bool demuxer,
                                                                        QList<QMediaFormat::AudioCodec> supportedAudioCodecs,
                                                                        QList<QMediaFormat::VideoCodec> supportedVideoCodecs)
{
    QList<QGstreamerFormatInfo::CodecMap> muxers;

    GstPadDirection padDirection = demuxer ? GST_PAD_SINK : GST_PAD_SRC;

    GList *elementList = gst_element_factory_list_get_elements(demuxer ? GST_ELEMENT_FACTORY_TYPE_DEMUXER : GST_ELEMENT_FACTORY_TYPE_MUXER,
                                                               GST_RANK_MARGINAL);
    GList *element = elementList;
    while (element) {
        GstElementFactory *factory = (GstElementFactory *)element->data;
        element = element->next;

        QList<QMediaFormat::FileFormat> fileFormats;

        const GList *padTemplates = gst_element_factory_get_static_pad_templates(factory);
        while (padTemplates) {
            GstStaticPadTemplate *padTemplate = (GstStaticPadTemplate *)padTemplates->data;
            padTemplates = padTemplates->next;

            if (padTemplate->direction == padDirection) {
                auto caps = QGstCaps(gst_static_caps_get(&padTemplate->static_caps), QGstCaps::HasRef);

                for (int i = 0; i < caps.size(); i++) {
                    QGstStructure structure = caps.at(i);
                    auto fmt = fileFormatForCaps(structure);
                    if (fmt != QMediaFormat::UnspecifiedFormat)
                        fileFormats.append(fmt);
                }
            }
        }
        if (fileFormats.isEmpty())
            continue;

        QList<QMediaFormat::AudioCodec> audioCodecs;
        QList<QMediaFormat::VideoCodec> videoCodecs;

        padTemplates = gst_element_factory_get_static_pad_templates(factory);
        while (padTemplates) {
            GstStaticPadTemplate *padTemplate = (GstStaticPadTemplate *)padTemplates->data;
            padTemplates = padTemplates->next;

            // check the other side for supported inputs/outputs
            if (padTemplate->direction != padDirection) {
                auto caps = QGstCaps(gst_static_caps_get(&padTemplate->static_caps), QGstCaps::HasRef);

                bool acceptsRawAudio = false;
                for (int i = 0; i < caps.size(); i++) {
                    QGstStructure structure = caps.at(i);
                    if (structure.name() == "audio/x-raw")
                        acceptsRawAudio = true;
                    auto audio = audioCodecForCaps(structure);
                    if (audio != QMediaFormat::AudioCodec::Unspecified && supportedAudioCodecs.contains(audio))
                        audioCodecs.append(audio);
                    auto video = videoCodecForCaps(structure);
                    if (video != QMediaFormat::VideoCodec::Unspecified && supportedVideoCodecs.contains(video))
                        videoCodecs.append(video);
                }
                if (acceptsRawAudio && fileFormats.size() == 1) {
                    switch (fileFormats.at(0)) {
                    case QMediaFormat::Mpeg4Audio:
                    default:
                        break;
                    case QMediaFormat::MP3:
                        audioCodecs.append(QMediaFormat::AudioCodec::MP3);
                        break;
                    case QMediaFormat::FLAC:
                        audioCodecs.append(QMediaFormat::AudioCodec::FLAC);
                        break;
                    case QMediaFormat::Wave:
                        audioCodecs.append(QMediaFormat::AudioCodec::Wave);
                        break;
                    }
                }
            }
        }
        if (!audioCodecs.isEmpty() || !videoCodecs.isEmpty()) {
            for (auto f : std::as_const(fileFormats)) {
                muxers.append({f, audioCodecs, videoCodecs});
                if (f == QMediaFormat::MPEG4 && !fileFormats.contains(QMediaFormat::Mpeg4Audio)) {
                    muxers.append({QMediaFormat::Mpeg4Audio, audioCodecs, {}});
                    if (audioCodecs.contains(QMediaFormat::AudioCodec::AAC))
                        muxers.append({QMediaFormat::AAC, { QMediaFormat::AudioCodec::AAC }, {}});
                } else if (f == QMediaFormat::WMV && !fileFormats.contains(QMediaFormat::WMA)) {
                    muxers.append({QMediaFormat::WMA, audioCodecs, {}});
                }
            }
        }
    }
    gst_plugin_feature_list_free(elementList);
    return muxers;
}

static QList<QImageCapture::FileFormat> getImageFormatList()
{
    QSet<QImageCapture::FileFormat> formats;

    GList *elementList = gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_ENCODER,
                                                               GST_RANK_MARGINAL);

    GList *element = elementList;
    while (element) {
        GstElementFactory *factory = (GstElementFactory *)element->data;
        element = element->next;

        const GList *padTemplates = gst_element_factory_get_static_pad_templates(factory);
        while (padTemplates) {
            GstStaticPadTemplate *padTemplate = (GstStaticPadTemplate *)padTemplates->data;
            padTemplates = padTemplates->next;

            if (padTemplate->direction == GST_PAD_SRC) {
                QGstCaps caps = QGstCaps(gst_static_caps_get(&padTemplate->static_caps), QGstCaps::HasRef);

                for (int i = 0; i < caps.size(); i++) {
                    QGstStructure structure = caps.at(i);
                    auto f = QGstreamerFormatInfo::imageFormatForCaps(structure);
                    if (f != QImageCapture::UnspecifiedFormat) {
//                        qDebug() << structure.toString() << f;
                        formats.insert(f);
                    }
                }
            }
        }
    }
    gst_plugin_feature_list_free(elementList);
    return formats.values();
}

#if 0
static void dumpAudioCodecs(const QList<QMediaFormat::AudioCodec> &codecList)
{
    qDebug() << "Audio codecs:";
    for (const auto &c : codecList)
        qDebug() << "            " << QMediaFormat::audioCodecName(c);
}

static void dumpVideoCodecs(const QList<QMediaFormat::VideoCodec> &codecList)
{
    qDebug() << "Video codecs:";
    for (const auto &c : codecList)
        qDebug() << "            " << QMediaFormat::videoCodecName(c);
}

static void dumpMuxers(const QList<QPlatformMediaFormatInfo::CodecMap> &muxerList)
{
    for (const auto &m : muxerList) {
        qDebug() << "    " << QMediaFormat::fileFormatName(m.format);
        qDebug() << "        Audio";
        for (const auto &a : m.audio)
            qDebug() << "            " << QMediaFormat::audioCodecName(a);
        qDebug() << "        Video";
        for (const auto &v : m.video)
            qDebug() << "            " << QMediaFormat::videoCodecName(v);
    }

}
#endif

QGstreamerFormatInfo::QGstreamerFormatInfo()
{
    auto codecs = getCodecsList(/*decode = */ true);
    decoders = getMuxerList(true, codecs.first, codecs.second);

    codecs = getCodecsList(/*decode = */ false);
    encoders = getMuxerList(/* demuxer = */false, codecs.first, codecs.second);
//    dumpAudioCodecs(codecs.first);
//    dumpVideoCodecs(codecs.second);
//    dumpMuxers(encoders);

    imageFormats = getImageFormatList();
}

QGstreamerFormatInfo::~QGstreamerFormatInfo() = default;

QGstCaps QGstreamerFormatInfo::formatCaps(const QMediaFormat &f) const
{
    auto format = f.fileFormat();
    Q_ASSERT(format != QMediaFormat::UnspecifiedFormat);

    const char *capsForFormat[QMediaFormat::LastFileFormat + 1] = {
        "video/x-ms-asf", // WMV
        "video/x-msvideo", // AVI
        "video/x-matroska", // Matroska
        "video/quicktime, variant=(string)iso", // MPEG4
        "video/ogg", // Ogg
        "video/quicktime", // QuickTime
        "video/webm", // WebM
        "video/quicktime, variant=(string)iso", // Mpeg4Audio is the same is mp4...
        "video/quicktime, variant=(string)iso", // AAC is also an MP4 container
        "video/x-ms-asf", // WMA, same as WMV
        "audio/mpeg, mpegversion=(int)1, layer=(int)3", // MP3
        "audio/x-flac", // FLAC
        "audio/x-wav" // Wave
    };
    return QGstCaps(gst_caps_from_string(capsForFormat[format]), QGstCaps::HasRef);
}

QGstCaps QGstreamerFormatInfo::audioCaps(const QMediaFormat &f) const
{
    auto codec = f.audioCodec();
    if (codec == QMediaFormat::AudioCodec::Unspecified)
        return {};

    const char *capsForCodec[(int)QMediaFormat::AudioCodec::LastAudioCodec + 1] = {
        "audio/mpeg, mpegversion=(int)1, layer=(int)3", // MP3
        "audio/mpeg, mpegversion=(int)4", // AAC
        "audio/x-ac3", // AC3
        "audio/x-eac3", // EAC3
        "audio/x-flac", // FLAC
        "audio/x-true-hd", // DolbyTrueHD
        "audio/x-opus", // Opus
        "audio/x-vorbis", // Vorbis
        "audio/x-raw", // WAVE
        "audio/x-wma", // WMA
        "audio/x-alac", // ALAC
    };
    return QGstCaps(gst_caps_from_string(capsForCodec[(int)codec]), QGstCaps::HasRef);
}

QGstCaps QGstreamerFormatInfo::videoCaps(const QMediaFormat &f) const
{
    auto codec = f.videoCodec();
    if (codec == QMediaFormat::VideoCodec::Unspecified)
        return {};

    const char *capsForCodec[(int)QMediaFormat::VideoCodec::LastVideoCodec + 1] = {
        "video/mpeg, mpegversion=(int)1", // MPEG1,
        "video/mpeg, mpegversion=(int)2", // MPEG2,
        "video/mpeg, mpegversion=(int)4", // MPEG4,
        "video/x-h264", // H264,
        "video/x-h265", // H265,
        "video/x-vp8", // VP8,
        "video/x-vp9", // VP9,
        "video/x-av1", // AV1,
        "video/x-theora", // Theora,
        "audio/x-wmv", // WMV
        "video/x-jpeg", // MotionJPEG,
    };
    return QGstCaps(gst_caps_from_string(capsForCodec[(int)codec]), QGstCaps::HasRef);
}

QT_END_NAMESPACE
