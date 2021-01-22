/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef MFAUDIODECODERCONTROL_H
#define MFAUDIODECODERCONTROL_H

#include "qaudiodecodercontrol.h"
#include "mfdecodersourcereader.h"
#include "sourceresolver.h"

QT_USE_NAMESPACE

class MFAudioDecoderControl : public QAudioDecoderControl
{
    Q_OBJECT
public:
    MFAudioDecoderControl(QObject *parent = 0);
    ~MFAudioDecoderControl();

    QAudioDecoder::State state() const;

    QString sourceFilename() const;
    void setSourceFilename(const QString &fileName);

    QIODevice* sourceDevice() const;
    void setSourceDevice(QIODevice *device);

    void start();
    void stop();

    QAudioFormat audioFormat() const;
    void setAudioFormat(const QAudioFormat &format);

    QAudioBuffer read();
    bool bufferAvailable() const;

    qint64 position() const;
    qint64 duration() const;

private Q_SLOTS:
    void handleMediaSourceReady();
    void handleMediaSourceError(long hr);
    void handleSampleAdded();
    void handleSourceFinished();

private:
    void updateResamplerOutputType();
    void activatePipeline();
    void onSourceCleared();

    MFDecoderSourceReader  *m_decoderSourceReader;
    SourceResolver         *m_sourceResolver;
    IMFTransform           *m_resampler;
    QAudioDecoder::State    m_state;
    QString                 m_sourceFilename;
    QIODevice              *m_device;
    QAudioFormat            m_audioFormat;
    DWORD                   m_mfInputStreamID;
    DWORD                   m_mfOutputStreamID;
    bool                    m_bufferReady;
    QAudioBuffer            m_cachedAudioBuffer;
    qint64                  m_duration;
    qint64                  m_position;
    bool                    m_loadingSource;
    IMFMediaType           *m_mfOutputType;
    IMFSample              *m_convertSample;
    QAudioFormat            m_sourceOutputFormat;
    bool                    m_sourceReady;
    bool                    m_resamplerDirty;
};

#endif//MFAUDIODECODERCONTROL_H
