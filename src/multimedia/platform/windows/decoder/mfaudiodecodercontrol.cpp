/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "Wmcodecdsp.h"
#include "mfaudiodecodercontrol_p.h"

MFAudioDecoderControl::MFAudioDecoderControl(QAudioDecoder *parent)
    : QPlatformAudioDecoder(parent)
    , m_decoderSourceReader(new MFDecoderSourceReader)
    , m_sourceResolver(new SourceResolver)
    , m_device(0)
    , m_mfInputStreamID(0)
    , m_mfOutputStreamID(0)
    , m_bufferReady(false)
    , m_duration(0)
    , m_position(0)
    , m_loadingSource(false)
    , m_mfOutputType(0)
    , m_convertSample(0)
    , m_sourceReady(false)
{
    connect(m_sourceResolver, SIGNAL(mediaSourceReady()), this, SLOT(handleMediaSourceReady()));
    connect(m_sourceResolver, SIGNAL(error(long)), this, SLOT(handleMediaSourceError(long)));
    connect(m_decoderSourceReader, SIGNAL(finished()), this, SLOT(handleSourceFinished()));
}

MFAudioDecoderControl::~MFAudioDecoderControl()
{
    if (m_mfOutputType)
        m_mfOutputType->Release();
    m_decoderSourceReader->shutdown();
    m_decoderSourceReader->Release();
    m_sourceResolver->Release();
}

QUrl MFAudioDecoderControl::source() const
{
    return m_source;
}

void MFAudioDecoderControl::onSourceCleared()
{
    bool positionDirty = false;
    bool durationDirty = false;
    if (m_position != 0) {
        m_position = 0;
        positionDirty = true;
    }
    if (m_duration != 0) {
        m_duration = 0;
        durationDirty = true;
    }
    if (positionDirty)
        positionChanged(m_position);
    if (durationDirty)
        durationChanged(m_duration);
}

void MFAudioDecoderControl::setSource(const QUrl &fileName)
{
    if (!m_device && m_source == fileName)
        return;
    m_sourceReady = false;
    m_sourceResolver->cancel();
    m_decoderSourceReader->setSource(nullptr);
    m_device = 0;
    m_source = fileName;
    if (!m_source.isEmpty()) {
        m_sourceResolver->shutdown();
        m_sourceResolver->load(m_source, 0);
        m_loadingSource = true;
    } else {
        onSourceCleared();
    }
    sourceChanged();
}

QIODevice* MFAudioDecoderControl::sourceDevice() const
{
    return m_device;
}

void MFAudioDecoderControl::setSourceDevice(QIODevice *device)
{
    if (m_device == device && m_source.isEmpty())
        return;
    m_sourceReady = false;
    m_sourceResolver->cancel();
    m_decoderSourceReader->setSource(nullptr);
    m_source.clear();
    m_device = device;
    if (m_device) {
        m_sourceResolver->shutdown();
        m_sourceResolver->load(QUrl(), m_device);
        m_loadingSource = true;
    } else {
        onSourceCleared();
    }
    sourceChanged();
}

void MFAudioDecoderControl::handleMediaSourceReady()
{
    m_loadingSource = false;
    m_sourceReady = true;
    IMFMediaType *mediaType = m_decoderSourceReader->setSource(m_sourceResolver->mediaSource());
    m_sourceOutputFormat = QAudioFormat();

    if (mediaType) {
        UINT32 val = 0;
        if (SUCCEEDED(mediaType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &val))) {
            m_sourceOutputFormat.setChannelCount(int(val));
        }
        if (SUCCEEDED(mediaType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &val))) {
            m_sourceOutputFormat.setSampleRate(int(val));
        }
        UINT32 bitsPerSample = 0;
        mediaType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bitsPerSample);

        GUID subType;
        if (SUCCEEDED(mediaType->GetGUID(MF_MT_SUBTYPE, &subType))) {
            if (subType == MFAudioFormat_Float) {
                m_sourceOutputFormat.setSampleFormat(QAudioFormat::Float);
            } else if (bitsPerSample == 8) {
                m_sourceOutputFormat.setSampleFormat(QAudioFormat::UInt8);
            } else if (bitsPerSample == 16) {
                m_sourceOutputFormat.setSampleFormat(QAudioFormat::Int16);
            } else if (bitsPerSample == 32){
                m_sourceOutputFormat.setSampleFormat(QAudioFormat::Int32);
            }
        }
    }

    if (m_sourceResolver->mediaSource()) {
        IMFPresentationDescriptor *pd;
        if (SUCCEEDED(m_sourceResolver->mediaSource()->CreatePresentationDescriptor(&pd))) {
            UINT64 duration = 0;
            pd->GetUINT64(MF_PD_DURATION, &duration);
            pd->Release();
            duration /= 10000;
            if (m_duration != qint64(duration)) {
                m_duration = qint64(duration);
                durationChanged(m_duration);
            }
        }
        if (isDecoding()) {
            activatePipeline();
        }
    } else if (isDecoding()) {
        setIsDecoding(false);
    }
}

void MFAudioDecoderControl::handleMediaSourceError(long hr)
{
    Q_UNUSED(hr);
    m_loadingSource = false;
    m_decoderSourceReader->setSource(nullptr);
    setIsDecoding(false);
}

void MFAudioDecoderControl::activatePipeline()
{
    Q_ASSERT(!m_bufferReady);
    setIsDecoding(true);
    connect(m_decoderSourceReader, SIGNAL(sampleAdded()), this, SLOT(handleSampleAdded()));
    m_decoderSourceReader->reset();
    m_decoderSourceReader->readNextSample();
    if (m_position != 0) {
        m_position = 0;
        positionChanged(0);
    }
}

void MFAudioDecoderControl::start()
{
    if (isDecoding())
        return;

    if (m_loadingSource) {
        //deferred starting
        setIsDecoding(true);
        return;
    }

    if (!m_decoderSourceReader->mediaSource())
        return;
    activatePipeline();
}

void MFAudioDecoderControl::stop()
{
    if (!isDecoding())
        return;
    disconnect(m_decoderSourceReader, SIGNAL(sampleAdded()), this, SLOT(handleSampleAdded()));
    if (m_bufferReady) {
        m_bufferReady = false;
        emit bufferAvailableChanged(m_bufferReady);
    }
    setIsDecoding(false);
}

void MFAudioDecoderControl::handleSampleAdded()
{
    QList<IMFSample*> samples = m_decoderSourceReader->takeSamples();
    Q_ASSERT(samples.count() > 0);
    Q_ASSERT(!m_bufferReady);
    LONGLONG sampleStartTime = 0;
    IMFSample *firstSample = samples.first();
    firstSample->GetSampleTime(&sampleStartTime);
    QByteArray abuf;
    for (IMFSample *s : qAsConst(samples)) {
        IMFMediaBuffer *buffer;
        s->ConvertToContiguousBuffer(&buffer);
        DWORD bufLen = 0;
        BYTE *buf = 0;
        if (SUCCEEDED(buffer->Lock(&buf, NULL, &bufLen))) {
            abuf.push_back(QByteArray(reinterpret_cast<char*>(buf), bufLen));
            buffer->Unlock();
        }
        buffer->Release();
        LONGLONG sampleTime = 0, sampleDuration = 0;
        s->GetSampleTime(&sampleTime);
        s->GetSampleDuration(&sampleDuration);
        m_position = qint64(sampleTime + sampleDuration) / 10000;
        s->Release();
    }

    // WMF uses 100-nanosecond units, QAudioDecoder uses milliseconds, QAudioBuffer uses microseconds...
    m_cachedAudioBuffer = QAudioBuffer(abuf, m_sourceOutputFormat, qint64(sampleStartTime / 10));
    m_bufferReady = true;
    emit positionChanged(m_position);
    emit bufferAvailableChanged(m_bufferReady);
    emit bufferReady();
}

void MFAudioDecoderControl::handleSourceFinished()
{
    stop();
    emit finished();
}

QAudioBuffer MFAudioDecoderControl::read()
{
    if (!m_bufferReady)
        return QAudioBuffer();
    QAudioBuffer buffer = m_cachedAudioBuffer;
    m_bufferReady = false;
    emit bufferAvailableChanged(m_bufferReady);
    m_decoderSourceReader->readNextSample();
    return buffer;
}

bool MFAudioDecoderControl::bufferAvailable() const
{
    return m_bufferReady;
}

qint64 MFAudioDecoderControl::position() const
{
    return m_position;
}

qint64 MFAudioDecoderControl::duration() const
{
    return m_duration;
}
