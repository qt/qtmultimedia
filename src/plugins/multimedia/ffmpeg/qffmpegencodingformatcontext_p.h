// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGENCODINGFORMATCONTEXT_P_H
#define QFFMPEGENCODINGFORMATCONTEXT_P_H

#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegdefs_p.h>

#include <QtMultimedia/qmediaformat.h>

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

QT_BEGIN_NAMESPACE

class QIODevice;
class QFile;

namespace QFFmpeg {

class EncodingFormatContext
{
public:
    explicit EncodingFormatContext(QMediaFormat::FileFormat fileFormat);
    ~EncodingFormatContext();

    [[nodiscard]] bool openAVIO(const QString &filePath);
    [[nodiscard]] bool openAVIO(QIODevice *device);
    bool isAVIOOpen() const { return m_avFormatContext->pb != nullptr; }
    void closeAVIO();

    AVFormatContext *avFormatContext() { return m_avFormatContext.get(); }

    const AVFormatContext *avFormatContext() const { return m_avFormatContext.get(); }

private:
    Q_DISABLE_COPY_MOVE(EncodingFormatContext)

    [[nodiscard]] bool openAVIOWithQFile(const QString &filePath);

private:
    AVMuxerContextUPtr m_avFormatContext;
    std::unique_ptr<QFile> m_outputFile;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGENCODINGFORMATCONTEXT_P_H
