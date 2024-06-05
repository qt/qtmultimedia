// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegencodingformatcontext_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegioutils_p.h"
#include "qfile.h"
#include "QtCore/qloggingcategory.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

Q_STATIC_LOGGING_CATEGORY(qLcEncodingFormatContext, "qt.multimedia.ffmpeg.encodingformatcontext");

namespace {
// In the example https://ffmpeg.org/doxygen/trunk/avio_read_callback_8c-example.html,
// BufferSize = 4096 is suggested, however, it might be not optimal. To be investigated.
constexpr size_t DefaultBufferSize = 4096;
} // namespace

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
    Q_ASSERT(!filePath.isEmpty());

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
    else
        openAVIOWithQFile(filePath);
}

void EncodingFormatContext::openAVIOWithQFile(const QString &filePath)
{
    // QTBUG-123082, To be investigated:
    // - should we use the logic with QFile for all file paths?
    // - does avio_open2 handle network protocols that QFile doesn't?
    // - which buffer size should we set to opening with QFile to ensure the best performance?

    auto file = std::make_unique<QFile>(filePath);

    if (!file->open(QFile::WriteOnly)) {
        qCDebug(qLcEncodingFormatContext) << "Cannot open QFile" << filePath;
        return;
    }

    openAVIO(file.get());

    if (isAVIOOpen())
        m_outputFile = std::move(file);
}

void EncodingFormatContext::openAVIO(QIODevice *device)
{
    Q_ASSERT(!isAVIOOpen());
    Q_ASSERT(device);

    if (!device->isWritable())
        return;

    auto buffer = static_cast<uint8_t *>(av_malloc(DefaultBufferSize));
    m_avFormatContext->pb = avio_alloc_context(buffer, DefaultBufferSize, 1, device, nullptr,
                                               &writeQIODevice, &seekQIODevice);
}

void EncodingFormatContext::closeAVIO()
{
    // Close the AVIOContext and release any file handles
    if (isAVIOOpen()) {
        if (m_avFormatContext->url && *m_avFormatContext->url != '\0') {
            auto closeResult = avio_closep(&m_avFormatContext->pb);
            Q_ASSERT(closeResult == 0);
        } else {
            av_free(std::exchange(m_avFormatContext->pb->buffer, nullptr));
            avio_context_free(&m_avFormatContext->pb);
        }

        // delete url even though it might be delete by avformat_free_context to
        // ensure consistency in openAVIO/closeAVIO.
        av_freep(&m_avFormatContext->url);
        m_outputFile.reset();
    } else {
        Q_ASSERT(!m_outputFile);
    }
}

} // namespace QFFmpeg

QT_END_NAMESPACE
