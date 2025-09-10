// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFmpegMediaFormatInfo_H
#define QFFmpegMediaFormatInfo_H

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

#include <QtMultimedia/private/qplatformmediaformatinfo_p.h>
#include <qhash.h>
#include <qlist.h>
#include <qaudioformat.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>

QT_BEGIN_NAMESPACE

class QFFmpegMediaFormatInfo : public QPlatformMediaFormatInfo
{
public:
    QFFmpegMediaFormatInfo();
    ~QFFmpegMediaFormatInfo() override;

    static QMediaFormat::VideoCodec videoCodecForAVCodecId(AVCodecID id);
    static QMediaFormat::AudioCodec audioCodecForAVCodecId(AVCodecID id);
    static QMediaFormat::FileFormat fileFormatForAVInputFormat(const AVInputFormat &format);

    static const AVOutputFormat *outputFormatForFileFormat(QMediaFormat::FileFormat format);

    static AVCodecID codecIdForVideoCodec(QMediaFormat::VideoCodec codec);
    static AVCodecID codecIdForAudioCodec(QMediaFormat::AudioCodec codec);

    static QAudioFormat::SampleFormat sampleFormat(AVSampleFormat format);
    static AVSampleFormat avSampleFormat(QAudioFormat::SampleFormat format);

    static int64_t avChannelLayout(QAudioFormat::ChannelConfig channelConfig);
    static QAudioFormat::ChannelConfig channelConfigForAVLayout(int64_t avChannelLayout);

    static QAudioFormat audioFormatFromCodecParameters(const AVCodecParameters &codecPar);
};

QT_END_NAMESPACE

#endif
