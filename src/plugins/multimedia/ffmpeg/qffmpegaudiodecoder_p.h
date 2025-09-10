// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGAUDIODECODER_H
#define QFFMPEGAUDIODECODER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/private/qplatformaudiodecoder_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>
#include <qurl.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {
class AudioDecoder;
} // namespace QFFmpeg

class QFFmpegAudioDecoder : public QPlatformAudioDecoder
{
    Q_OBJECT

public:
    QFFmpegAudioDecoder(QAudioDecoder *parent);
    ~QFFmpegAudioDecoder() override;

    QUrl source() const override;
    void setSource(const QUrl &fileName) override;

    QIODevice *sourceDevice() const override;
    void setSourceDevice(QIODevice *device) override;

    void start() override;
    void stop() override;

    QAudioFormat audioFormat() const override;
    void setAudioFormat(const QAudioFormat &format) override;

    QAudioBuffer read() override;

public Q_SLOTS:
    void newAudioBuffer(const QAudioBuffer &b);
    void done();
    void errorSignal(int err, const QString &errorString);

private:
    using AudioDecoder = QFFmpeg::AudioDecoder;

    QUrl m_url;
    QIODevice *m_sourceDevice = nullptr;
    std::unique_ptr<AudioDecoder> m_decoder;
    QAudioFormat m_audioFormat;

    QAudioBuffer m_audioBuffer;
};

QT_END_NAMESPACE

#endif // QFFMPEGAUDIODECODER_H
