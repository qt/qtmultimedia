// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegencodingformatcontext_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "QtCore/qloggingcategory.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

static Q_LOGGING_CATEGORY(qLcEncodingFormatContext, "qt.multimedia.ffmpeg.encodingformatcontext");

EncodingFormatContext::EncodingFormatContext(QMediaFormat::FileFormat fileFormat)
    : m_avFormatContext(avformat_alloc_context())
{
    const AVOutputFormat *avFormat = QFFmpegMediaFormatInfo::outputFormatForFileFormat(fileFormat);
    m_avFormatContext->oformat = const_cast<AVOutputFormat *>(avFormat); // constness varies
}

EncodingFormatContext::~EncodingFormatContext()
{
    closeAVIO();

    avformat_free_context(m_avFormatContext);
}

void EncodingFormatContext::openAVIO(const QString &filePath)
{
    Q_ASSERT(!isAVIOOpen());

    const QByteArray filePathUtf8 = filePath.toUtf8();

    std::unique_ptr<char, decltype(&av_free)> url(
            reinterpret_cast<char *>(av_malloc(filePathUtf8.size() + 1)), &av_free);
    memcpy(url.get(), filePathUtf8.constData(), filePathUtf8.size() + 1);

    // Initialize the AVIOContext for accessing the resource indicated by the url
    auto result = avio_open2(&m_avFormatContext->pb, url.get(), AVIO_FLAG_WRITE, nullptr, nullptr);

    qCDebug(qLcEncodingFormatContext)
            << "opened by file path:" << url.get() << ", result:" << result;

    Q_ASSERT(m_avFormatContext->url == nullptr);
    if (isAVIOOpen())
        m_avFormatContext->url = url.release();
}

void EncodingFormatContext::closeAVIO()
{
    // Close the AVIOContext and release any file handles
    if (auto io = std::exchange(m_avFormatContext->pb, nullptr)) {
        const int res = avio_close(io);
        Q_ASSERT(res == 0);

        // delete url even though it might be delete by avformat_free_context to
        // ensure consistency in openAVIO/closeAVIO.
        av_freep(&m_avFormatContext->url);
    }
}

} // namespace QFFmpeg

QT_END_NAMESPACE
