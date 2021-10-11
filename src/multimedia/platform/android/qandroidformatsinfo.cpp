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

#include "qandroidformatsinfo_p.h"

#include <QMutableListIterator>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>
#include <qcoreapplication.h>

static const char encoderFilter[] = ".encoder";
static const char decoderFilter[] = ".decoder";

QT_BEGIN_NAMESPACE

QAndroidFormatInfo::QAndroidFormatInfo()
{
    // Audio/Video/Image formats with their decoder/encoder information is documented at
    // https://developer.android.com/guide/topics/media/media-formats

    const QJniObject codecsArrayObject = QJniObject::callStaticObjectMethod(
                "org/qtproject/qt/android/multimedia/QtMultimediaUtils",
                "getMediaCodecs",
                "()[Ljava/lang/String;");
    QStringList codecs;
    QJniEnvironment env;
    const jobjectArray devsArray = codecsArrayObject.object<jobjectArray>();
    for (int i = 0; i < env->GetArrayLength(devsArray); ++i) {
        const QString codec = QJniObject(env->GetObjectArrayElement(devsArray, i)).toString();
        if (codec.contains(QStringLiteral("encoder")))
            m_supportedEncoders.append(codec);
        else
            m_supportedDecoders.append(codec);
    }

    auto removeUnspecifiedValues = [](QList<CodecMap> &map) {
        QMutableListIterator<CodecMap> iter(map);
        while (iter.hasNext()) {
            CodecMap &codec = iter.next();
            codec.audio.removeAll(QMediaFormat::AudioCodec::Unspecified);
            codec.video.removeAll(QMediaFormat::VideoCodec::Unspecified);

            if (codec.audio.size() < 1 && codec.video.size() < 1)
                iter.remove();
        }
    };

    {
        const QMediaFormat::AudioCodec aac = hasDecoder(QMediaFormat::AudioCodec::AAC);
        const QMediaFormat::AudioCodec mp3 = hasDecoder(QMediaFormat::AudioCodec::MP3);
        const QMediaFormat::AudioCodec flac = hasDecoder(QMediaFormat::AudioCodec::FLAC);
        const QMediaFormat::AudioCodec opus = hasDecoder(QMediaFormat::AudioCodec::Opus);
        const QMediaFormat::AudioCodec vorbis = hasDecoder(QMediaFormat::AudioCodec::Vorbis);

        const QMediaFormat::VideoCodec vp8 = hasDecoder(QMediaFormat::VideoCodec::VP8);
        const QMediaFormat::VideoCodec vp9 = hasDecoder(QMediaFormat::VideoCodec::VP9);
        const QMediaFormat::VideoCodec h264 = hasDecoder(QMediaFormat::VideoCodec::H264);
        const QMediaFormat::VideoCodec h265 = hasDecoder(QMediaFormat::VideoCodec::H265);
        const QMediaFormat::VideoCodec av1 = hasDecoder(QMediaFormat::VideoCodec::AV1);

        decoders = {
            { QMediaFormat::AAC, {aac}, {} },
            { QMediaFormat::MP3, {mp3}, {} },
            { QMediaFormat::FLAC, {flac}, {} },
            { QMediaFormat::Mpeg4Audio, {mp3, aac, flac, vorbis}, {} },
            { QMediaFormat::MPEG4, {mp3, aac, flac, vorbis}, {h264, h265, av1} },
            { QMediaFormat::Ogg, {opus, vorbis, flac}, {} },
            { QMediaFormat::Matroska, {mp3, opus, vorbis}, {vp8, vp9, h264, h265, av1} },
            { QMediaFormat::WebM, {opus, vorbis}, {vp8, vp9} }
        };

        removeUnspecifiedValues(decoders);
    }

    {
        const QMediaFormat::AudioCodec aac = hasEncoder(QMediaFormat::AudioCodec::AAC);
        const QMediaFormat::AudioCodec mp3 = hasEncoder(QMediaFormat::AudioCodec::MP3);
        const QMediaFormat::AudioCodec flac = hasEncoder(QMediaFormat::AudioCodec::FLAC);
        const QMediaFormat::AudioCodec opus = hasEncoder(QMediaFormat::AudioCodec::Opus);
        const QMediaFormat::AudioCodec vorbis = hasEncoder(QMediaFormat::AudioCodec::Vorbis);

        const QMediaFormat::VideoCodec vp8 = hasEncoder(QMediaFormat::VideoCodec::VP8);
        const QMediaFormat::VideoCodec vp9 = hasEncoder(QMediaFormat::VideoCodec::VP9);
        const QMediaFormat::VideoCodec h264 = hasEncoder(QMediaFormat::VideoCodec::H264);
        const QMediaFormat::VideoCodec h265 = hasEncoder(QMediaFormat::VideoCodec::H265);
        const QMediaFormat::VideoCodec av1 = hasEncoder(QMediaFormat::VideoCodec::AV1);

        // MP3 and Vorbis encoders are not supported by the default Android SDK
        // Opus encoder available only for Android 10+
        encoders = {
            { QMediaFormat::AAC, {aac}, {} },
            { QMediaFormat::MP3, {mp3}, {} },
            { QMediaFormat::FLAC, {flac}, {} },
            { QMediaFormat::Mpeg4Audio, {mp3, aac, flac, vorbis}, {} },
            { QMediaFormat::MPEG4, {mp3, aac, flac, vorbis}, {h264, h265, av1} },
            { QMediaFormat::Ogg, {opus, vorbis, flac}, {} },
            { QMediaFormat::Matroska, {mp3, opus, flac}, {vp8, vp9, h264, h265, av1} },
            // NOTE: WebM seems to be documented to supported with VP8 encoder,
            // but the Camera API doesn't work with it, keep it commented for now.
            // { QMediaFormat::WebM, {vorbis, opus}, {vp8, vp9} }
        };

        removeUnspecifiedValues(encoders);
    }

    imageFormats << QImageCapture::JPEG;
    // NOTE: Add later if needed, the Camera API doens't seem to work with it.
    // imageFormats << QImageCapture::PNG << QImageCapture::WebP;
}

QAndroidFormatInfo::~QAndroidFormatInfo()
{
}

static QString getVideoCodecName(QMediaFormat::VideoCodec codec)
{
    QString codecString = QMediaFormat::videoCodecName(codec);
    if (codec == QMediaFormat::VideoCodec::H265)
        codecString = QLatin1String("HEVC");
    return codecString;
}

QMediaFormat::AudioCodec QAndroidFormatInfo::hasEncoder(QMediaFormat::AudioCodec codec) const
{
    const QString codecString = QMediaFormat::audioCodecName(codec);
    for (auto str : m_supportedEncoders) {
        if (str.contains(codecString + QLatin1String(encoderFilter), Qt::CaseInsensitive))
            return codec;
    }
    return QMediaFormat::AudioCodec::Unspecified;
}

QMediaFormat::VideoCodec QAndroidFormatInfo::hasEncoder(QMediaFormat::VideoCodec codec) const
{
    const QString codecString = getVideoCodecName(codec);
    for (auto str : m_supportedEncoders) {
        if (str.contains(codecString + QLatin1String(encoderFilter), Qt::CaseInsensitive))
            return codec;
    }
    return QMediaFormat::VideoCodec::Unspecified;
}

QMediaFormat::AudioCodec QAndroidFormatInfo::hasDecoder(QMediaFormat::AudioCodec codec) const
{
    const QString codecString = QMediaFormat::audioCodecName(codec);
    for (auto str : m_supportedDecoders) {
        if (str.contains(codecString + QLatin1String(decoderFilter), Qt::CaseInsensitive))
            return codec;
    }
    return QMediaFormat::AudioCodec::Unspecified;
}

QMediaFormat::VideoCodec QAndroidFormatInfo::hasDecoder(QMediaFormat::VideoCodec codec) const
{
    const QString codecString = getVideoCodecName(codec);
    for (auto str : m_supportedDecoders) {
        if (str.contains(codecString + QLatin1String(decoderFilter), Qt::CaseInsensitive))
            return codec;
    }
    return QMediaFormat::VideoCodec::Unspecified;
}

QT_END_NAMESPACE
