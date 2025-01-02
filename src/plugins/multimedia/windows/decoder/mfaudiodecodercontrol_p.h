// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef MFAUDIODECODERCONTROL_H
#define MFAUDIODECODERCONTROL_H

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

#include "mfdecodersourcereader_p.h"
#include <private/qplatformaudiodecoder_p.h>
#include <sourceresolver_p.h>
#include <private/qcomptr_p.h>
#include <private/qwindowsresampler_p.h>

QT_BEGIN_NAMESPACE

class MFAudioDecoderControl : public QPlatformAudioDecoder
{
    Q_OBJECT
public:
    MFAudioDecoderControl(QAudioDecoder *parent);
    ~MFAudioDecoderControl() override;

    QUrl source() const override { return m_source; }
    void setSource(const QUrl &fileName) override;

    QIODevice* sourceDevice() const override { return m_device; }
    void setSourceDevice(QIODevice *device) override;

    void start() override;
    void stop() override;

    QAudioFormat audioFormat() const override { return m_outputFormat; }
    void setAudioFormat(const QAudioFormat &format) override;

    QAudioBuffer read() override;
    bool bufferAvailable() const override { return m_audioBuffer.sampleCount() > 0; }

    qint64 position() const override { return m_position; }
    qint64 duration() const override { return m_duration; }

private Q_SLOTS:
    void handleMediaSourceReady();
    void handleMediaSourceError(long hr);
    void handleNewSample(ComPtr<IMFSample>);
    void handleSourceFinished();

private:
    void startReadingSource(IMFMediaSource *source);

    ComPtr<MFDecoderSourceReader>  m_decoderSourceReader;
    SourceResolver         *m_sourceResolver;
    QWindowsResampler       m_resampler;
    QUrl                    m_source;
    QIODevice              *m_device = nullptr;
    QAudioFormat            m_outputFormat;
    QAudioBuffer            m_audioBuffer;
    qint64                  m_duration = -1;
    qint64                  m_position = -1;
    bool                    m_loadingSource = false;
    bool                    m_deferredStart = false;
};

QT_END_NAMESPACE

#endif//MFAUDIODECODERCONTROL_H
