// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <common/qgstutils_p.h>
#include <common/qgst_p.h>

#include <QtMultimedia/qaudioformat.h>

#include <chrono>

QT_BEGIN_NAMESPACE

namespace {

const char *audioSampleFormatNames[QAudioFormat::NSampleFormats] = {
    nullptr,
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    "U8",
    "S16LE",
    "S32LE",
    "F32LE"
#else
    "U8",
    "S16BE",
    "S32BE",
    "F32BE"
#endif
};

QAudioFormat::SampleFormat gstSampleFormatToSampleFormat(const char *fmt)
{
    if (fmt) {
        for (int i = 1; i < QAudioFormat::NSampleFormats; ++i) {
            if (strcmp(fmt, audioSampleFormatNames[i]))
                continue;
            return QAudioFormat::SampleFormat(i);
        }
    }
    return QAudioFormat::Unknown;
}

} // namespace

/*
  Returns audio format for a sample \a sample.
  If the buffer doesn't have a valid audio format, an empty QAudioFormat is returned.
*/
QAudioFormat QGstUtils::audioFormatForSample(GstSample *sample)
{
    auto caps = QGstCaps(gst_sample_get_caps(sample), QGstCaps::NeedsRef);
    if (!caps)
        return {};
    return audioFormatForCaps(caps);
}

QAudioFormat QGstUtils::audioFormatForCaps(const QGstCaps &caps)
{
    QAudioFormat format;
    QGstStructureView s = caps.at(0);
    if (s.name() != "audio/x-raw")
        return format;

    auto rate = s["rate"].toInt();
    auto channels = s["channels"].toInt();
    QAudioFormat::SampleFormat fmt = gstSampleFormatToSampleFormat(s["format"].toString());
    if (!rate || !channels || fmt == QAudioFormat::Unknown)
        return format;

    format.setSampleRate(*rate);
    format.setChannelCount(*channels);
    format.setSampleFormat(fmt);

    return format;
}

/*
  Builds GstCaps for an audio format \a format.
  Returns 0 if the audio format is not valid.

  \note Caller must unreference GstCaps.
*/

QGstCaps QGstUtils::capsForAudioFormat(const QAudioFormat &format)
{
    if (!format.isValid())
        return {};

    auto sampleFormat = format.sampleFormat();
    auto caps = gst_caps_new_simple(
                "audio/x-raw",
                "format"  , G_TYPE_STRING, audioSampleFormatNames[sampleFormat],
                "rate"    , G_TYPE_INT   , format.sampleRate(),
                "channels", G_TYPE_INT   , format.channelCount(),
                "layout"  , G_TYPE_STRING, "interleaved",
                nullptr);

    return QGstCaps(caps, QGstCaps::HasRef);
}

QList<QAudioFormat::SampleFormat> QGValue::getSampleFormats() const
{
    if (!GST_VALUE_HOLDS_LIST(value))
        return {};

    QList<QAudioFormat::SampleFormat> formats;
    guint nFormats = gst_value_list_get_size(value);
    for (guint f = 0; f < nFormats; ++f) {
        QGValue v = QGValue{ gst_value_list_get_value(value, f) };
        auto *name = v.toString();
        QAudioFormat::SampleFormat fmt = gstSampleFormatToSampleFormat(name);
        if (fmt == QAudioFormat::Unknown)
            continue;
        formats.append(fmt);
    }
    return formats;
}

void QGstUtils::setFrameTimeStampsFromBuffer(QVideoFrame *frame, GstBuffer *buffer)
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    // GStreamer uses nanoseconds, Qt uses microseconds
    nanoseconds startTime{ GST_BUFFER_TIMESTAMP(buffer) };
    if (startTime >= 0ns) {
        frame->setStartTime(floor<microseconds>(startTime).count());

        nanoseconds duration{ GST_BUFFER_DURATION(buffer) };
        if (duration >= 0ns)
            frame->setEndTime(floor<microseconds>(startTime + duration).count());
    }
}

GList *qt_gst_video_sinks()
{
    return gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_SINK
                                                         | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO,
                                                 GST_RANK_MARGINAL);
}

QLocale::Language QGstUtils::codeToLanguage(const gchar *lang)
{
    return QLocale::codeToLanguage(QString::fromUtf8(lang), QLocale::AnyLanguageCode);
}

QT_END_NAMESPACE
