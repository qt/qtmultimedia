// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamerformatinfo_p.h"

#include <common/qglist_helper_p.h>

#include <QtCore/qset.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE

QMediaFormat::AudioCodec QGstreamerFormatInfo::audioCodecForCaps(QGstStructureView structure)
{
    using namespace std::string_view_literals;
    const char *name = structure.name().data();

    if (!name || (strncmp(name, "audio/", 6) != 0))
        return QMediaFormat::AudioCodec::Unspecified;
    name += 6;
    if (name == "mpeg"sv) {
        auto version = structure["mpegversion"].toInt();
        if (version == 1) {
            auto layer = structure["layer"];
            if (!layer.isNull())
                return QMediaFormat::AudioCodec::MP3;
        }
        if (version == 4)
            return QMediaFormat::AudioCodec::AAC;
        return QMediaFormat::AudioCodec::Unspecified;
    }
    if (name == "x-ac3"sv)
        return QMediaFormat::AudioCodec::AC3;

    if (name == "x-eac3"sv)
        return QMediaFormat::AudioCodec::EAC3;

    if (name == "x-flac"sv)
        return QMediaFormat::AudioCodec::FLAC;

    if (name == "x-alac"sv)
        return QMediaFormat::AudioCodec::ALAC;

    if (name == "x-true-hd"sv)
        return QMediaFormat::AudioCodec::DolbyTrueHD;

    if (name == "x-vorbis"sv)
        return QMediaFormat::AudioCodec::Vorbis;

    if (name == "x-opus"sv)
        return QMediaFormat::AudioCodec::Opus;

    if (name == "x-wav"sv)
        return QMediaFormat::AudioCodec::Wave;

    if (name == "x-wma"sv)
        return QMediaFormat::AudioCodec::WMA;

    return QMediaFormat::AudioCodec::Unspecified;
}

QMediaFormat::VideoCodec QGstreamerFormatInfo::videoCodecForCaps(QGstStructureView structure)
{
    using namespace std::string_view_literals;
    const char *name = structure.name().data();

    if (!name || (strncmp(name, "video/", 6) != 0))
        return QMediaFormat::VideoCodec::Unspecified;
    name += 6;

    if (name == "mpeg"sv) {
        auto version = structure["mpegversion"].toInt();
        if (version == 1)
            return QMediaFormat::VideoCodec::MPEG1;
        if (version == 2)
            return QMediaFormat::VideoCodec::MPEG2;
        if (version == 4)
            return QMediaFormat::VideoCodec::MPEG4;
        return QMediaFormat::VideoCodec::Unspecified;
    }
    if (name == "x-h264"sv)
        return QMediaFormat::VideoCodec::H264;

    if (name == "x-h265"sv)
        return QMediaFormat::VideoCodec::H265;

    if (name == "x-vp8"sv)
        return QMediaFormat::VideoCodec::VP8;

    if (name == "x-vp9"sv)
        return QMediaFormat::VideoCodec::VP9;

    if (name == "x-av1"sv)
        return QMediaFormat::VideoCodec::AV1;

    if (name == "x-theora"sv)
        return QMediaFormat::VideoCodec::Theora;

    if (name == "x-jpeg"sv)
        return QMediaFormat::VideoCodec::MotionJPEG;

    if (name == "x-wmv"sv)
        return QMediaFormat::VideoCodec::WMV;

    return QMediaFormat::VideoCodec::Unspecified;
}

QMediaFormat::FileFormat QGstreamerFormatInfo::fileFormatForCaps(QGstStructureView structure)
{
    using namespace std::string_view_literals;
    const char *name = structure.name().data();

    if (name == "video/x-ms-asf"sv)
        return QMediaFormat::FileFormat::WMV;

    if (name == "video/x-msvideo"sv)
        return QMediaFormat::FileFormat::AVI;

    if (name == "video/x-matroska"sv)
        return QMediaFormat::FileFormat::Matroska;

    if (name == "video/quicktime"sv) {
        const char *variant = structure["variant"].toString();
        if (!variant)
            return QMediaFormat::FileFormat::QuickTime;
        if (variant == "iso"sv)
            return QMediaFormat::FileFormat::MPEG4;
    }
    if (name == "video/ogg"sv)
        return QMediaFormat::FileFormat::Ogg;

    if (name == "video/webm"sv)
        return QMediaFormat::FileFormat::WebM;

    if (name == "audio/x-m4a"sv)
        return QMediaFormat::FileFormat::Mpeg4Audio;

    if (name == "audio/x-wav"sv)
        return QMediaFormat::FileFormat::Wave;

    if (name == "audio/mpeg"sv) {
        auto mpegversion = structure["mpegversion"].toInt();
        if (mpegversion == 1) {
            auto layer = structure["layer"];
            if (!layer.isNull())
                return QMediaFormat::FileFormat::MP3;
        }
    }

    if (name == "audio/aac"sv)
        return QMediaFormat::FileFormat::AAC;

    if (name == "audio/x-ms-wma"sv)
        return QMediaFormat::FileFormat::WMA;

    if (name == "audio/x-flac"sv)
        return QMediaFormat::FileFormat::FLAC;

    return QMediaFormat::UnspecifiedFormat;
}


QImageCapture::FileFormat QGstreamerFormatInfo::imageFormatForCaps(QGstStructureView structure)
{
    using namespace std::string_view_literals;
    const char *name = structure.name().data();

    if (name == "image/jpeg"sv)
        return QImageCapture::JPEG;

    if (name == "image/png"sv)
        return QImageCapture::PNG;

    if (name == "image/webp"sv)
        return QImageCapture::WebP;

    if (name == "image/tiff"sv)
        return QImageCapture::Tiff;

    return QImageCapture::UnspecifiedFormat;
}

static std::pair<QList<QMediaFormat::AudioCodec>, QList<QMediaFormat::VideoCodec>>
getCodecsList(bool decode)
{
    QList<QMediaFormat::AudioCodec> audio;
    QList<QMediaFormat::VideoCodec> video;

    GstPadDirection padDirection = decode ? GST_PAD_SINK : GST_PAD_SRC;

    GList *elementList = gst_element_factory_list_get_elements(decode ? GST_ELEMENT_FACTORY_TYPE_DECODER : GST_ELEMENT_FACTORY_TYPE_ENCODER,
                                                               GST_RANK_MARGINAL);

    for (GstElementFactory *factory :
         QGstUtils::GListRangeAdaptor<GstElementFactory *>(elementList)) {
        for (GstStaticPadTemplate *padTemplate :
             QGstUtils::GListConstRangeAdaptor<GstStaticPadTemplate *>(
                     gst_element_factory_get_static_pad_templates(factory))) {
            if (padTemplate->direction == padDirection) {
                auto caps = QGstCaps(gst_static_caps_get(&padTemplate->static_caps), QGstCaps::HasRef);

                for (int i = 0; i < caps.size(); i++) {
                    QGstStructureView structure = caps.at(i);
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

QList<QGstreamerFormatInfo::CodecMap>
QGstreamerFormatInfo::getCodecMaps(QMediaFormat::ConversionMode conversionMode,
                                   QList<QMediaFormat::AudioCodec> supportedAudioCodecs,
                                   QList<QMediaFormat::VideoCodec> supportedVideoCodecs)
{
    QList<QGstreamerFormatInfo::CodecMap> maps;

    GstPadDirection dataPadDirection =
            (conversionMode == QMediaFormat::Decode) ? GST_PAD_SINK : GST_PAD_SRC;

    auto encodableFactoryTypes =
            (GST_ELEMENT_FACTORY_TYPE_MUXER | GST_ELEMENT_FACTORY_TYPE_PAYLOADER
             | GST_ELEMENT_FACTORY_TYPE_ENCRYPTOR | GST_ELEMENT_FACTORY_TYPE_ENCODER);
    GList *elementList = gst_element_factory_list_get_elements(
            (conversionMode == QMediaFormat::Decode) ? GST_ELEMENT_FACTORY_TYPE_DECODABLE
                                                     : encodableFactoryTypes,
            GST_RANK_MARGINAL);

    for (GstElementFactory *factory :
         QGstUtils::GListRangeAdaptor<GstElementFactory *>(elementList)) {
        QList<QMediaFormat::FileFormat> fileFormats;

        for (GstStaticPadTemplate *padTemplate :
             QGstUtils::GListConstRangeAdaptor<GstStaticPadTemplate *>(
                     gst_element_factory_get_static_pad_templates(factory))) {

            // Check pads on data side for file formats, except for parsers check source side
            if (padTemplate->direction == dataPadDirection
                || (gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_PARSER)
                    && padTemplate->direction == GST_PAD_SRC)) {
                auto caps = QGstCaps(gst_static_caps_get(&padTemplate->static_caps), QGstCaps::HasRef);

                for (int i = 0; i < caps.size(); i++) {
                    QGstStructureView structure = caps.at(i);
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

        for (GstStaticPadTemplate *padTemplate :
             QGstUtils::GListConstRangeAdaptor<GstStaticPadTemplate *>(
                     gst_element_factory_get_static_pad_templates(factory))) {

            // check the other side for supported inputs/outputs
            if (padTemplate->direction != dataPadDirection) {
                auto caps = QGstCaps(gst_static_caps_get(&padTemplate->static_caps), QGstCaps::HasRef);

                bool acceptsRawAudio = false;
                for (int i = 0; i < caps.size(); i++) {
                    QGstStructureView structure = caps.at(i);
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
                maps.append({f, audioCodecs, videoCodecs});
                if (f == QMediaFormat::MPEG4 && !fileFormats.contains(QMediaFormat::Mpeg4Audio)) {
                    maps.append({QMediaFormat::Mpeg4Audio, audioCodecs, {}});
                    if (audioCodecs.contains(QMediaFormat::AudioCodec::AAC))
                        maps.append({QMediaFormat::AAC, { QMediaFormat::AudioCodec::AAC }, {}});
                } else if (f == QMediaFormat::WMV && !fileFormats.contains(QMediaFormat::WMA)) {
                    maps.append({QMediaFormat::WMA, audioCodecs, {}});
                }
            }
        }
    }
    gst_plugin_feature_list_free(elementList);
    return maps;
}

static QList<QImageCapture::FileFormat> getImageFormatList()
{
    QSet<QImageCapture::FileFormat> formats;

    GList *elementList = gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_ENCODER,
                                                               GST_RANK_MARGINAL);

    for (GstElementFactory *factory :
         QGstUtils::GListRangeAdaptor<GstElementFactory *>(elementList)) {

        for (GstStaticPadTemplate *padTemplate :
             QGstUtils::GListConstRangeAdaptor<GstStaticPadTemplate *>(
                     gst_element_factory_get_static_pad_templates(factory))) {
            if (padTemplate->direction == GST_PAD_SRC) {
                QGstCaps caps = QGstCaps(gst_static_caps_get(&padTemplate->static_caps), QGstCaps::HasRef);

                for (int i = 0; i < caps.size(); i++) {
                    QGstStructureView structure = caps.at(i);
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
    decoders = getCodecMaps(QMediaFormat::Decode, codecs.first, codecs.second);

    codecs = getCodecsList(/*decode = */ false);
    encoders = getCodecMaps(QMediaFormat::Encode, codecs.first, codecs.second);
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
