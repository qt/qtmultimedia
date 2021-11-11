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

#include <system_error>
#include <mferror.h>
#include <qglobal.h>
#include "Wmcodecdsp.h"
#include "mfaudiodecodercontrol_p.h"
#include <private/qwindowsaudioutils_p.h>

MFAudioDecoderControl::MFAudioDecoderControl(QAudioDecoder *parent)
    : QPlatformAudioDecoder(parent)
    , m_sourceResolver(new SourceResolver)
{
    CoCreateInstance(CLSID_CResamplerMediaObject, NULL, CLSCTX_INPROC_SERVER, IID_IMFTransform, (LPVOID*)(m_resampler.address()));
    if (m_resampler)
        m_resampler->AddInputStreams(1, &m_mfInputStreamID);
    else
        qWarning("MFAudioDecoderControl: Failed to create resampler(CLSID_CResamplerMediaObject)");

    connect(m_sourceResolver, SIGNAL(mediaSourceReady()), this, SLOT(handleMediaSourceReady()));
    connect(m_sourceResolver, SIGNAL(error(long)), this, SLOT(handleMediaSourceError(long)));
}

MFAudioDecoderControl::~MFAudioDecoderControl()
{
    m_sourceResolver->shutdown();
    m_sourceResolver->Release();
}

void MFAudioDecoderControl::setSource(const QUrl &fileName)
{
    if (!m_device && m_source == fileName)
        return;
    stop();
    m_sourceResolver->cancel();
    m_sourceResolver->shutdown();
    m_device = nullptr;
    m_source = fileName;
    sourceChanged();

    if (!m_source.isEmpty()) {
        m_sourceResolver->load(m_source, 0);
        m_loadingSource = true;
    }
}

void MFAudioDecoderControl::setSourceDevice(QIODevice *device)
{
    if (m_device == device && m_source.isEmpty())
        return;
    stop();
    m_sourceResolver->cancel();
    m_sourceResolver->shutdown();
    m_source.clear();
    m_device = device;
    sourceChanged();

    if (m_device) {
        if (m_device->isOpen() && m_device->isReadable()) {
            m_sourceResolver->load(QUrl(), m_device);
            m_loadingSource = true;
        }
    }
}

HRESULT MFAudioDecoderControl::setResamplerOutputType(IMFMediaType *outputType)
{
    Q_ASSERT(m_resampler);

    HRESULT hr = m_resampler->SetOutputType(m_mfOutputStreamID, outputType, 0);
    if (FAILED(hr))
        return hr;

    MFT_OUTPUT_STREAM_INFO streamInfo;
    m_resampler->GetOutputStreamInfo(m_mfOutputStreamID, &streamInfo);
    if ((streamInfo.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES |  MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES)) == 0) {
        //if resampler does not allocate output sample memory, we do it here
        m_convertSample.reset();
        hr = MFCreateSample(m_convertSample.address());
        if (FAILED(hr))
            return hr;

        QWindowsIUPointer<IMFMediaBuffer> buffer;
        hr = MFCreateMemoryBuffer(qMax(streamInfo.cbSize, 10000u), buffer.address());
        if (FAILED(hr)) {
            m_convertSample.reset();
            return hr;
        }
        m_convertSample->AddBuffer(buffer.get());
    }
    return S_OK;
}

void MFAudioDecoderControl::handleMediaSourceReady()
{
    m_loadingSource = false;
    if (m_deferredStart) {
        m_deferredStart = false;
        startReadingSource(m_sourceResolver->mediaSource());
    }
}

void MFAudioDecoderControl::handleMediaSourceError(long hr)
{
    m_loadingSource = false;
    m_deferredStart = false;
    if (hr == MF_E_UNSUPPORTED_BYTESTREAM_TYPE) {
        error(QAudioDecoder::FormatError, tr("Unsupported media type"));
    } else if (hr == ERROR_FILE_NOT_FOUND) {
        error(QAudioDecoder::ResourceError, tr("Media not found"));
    } else {
        error(QAudioDecoder::ResourceError, tr("Unable to load specified URL")
                      + QString::fromStdString(std::system_category().message(hr)));
    }
}

void MFAudioDecoderControl::startReadingSource(IMFMediaSource *source)
{
    Q_ASSERT(source);

    m_decoderSourceReader.reset(new MFDecoderSourceReader());
    if (!m_decoderSourceReader) {
        error(QAudioDecoder::ResourceError, tr("Could not instantiate MFDecoderSourceReader"));
        return;
    }

    auto mediaType = m_decoderSourceReader->setSource(source, m_outputFormat.sampleFormat());
    m_mediaFormat = QWindowsAudioUtils::mediaTypeToFormat(mediaType.get());
    if (!m_mediaFormat.isValid()) {
        error(QAudioDecoder::FormatError, tr("Invalid media format"));
        m_decoderSourceReader.reset();
        return;
    }

    QWindowsIUPointer<IMFPresentationDescriptor> pd;
    if (SUCCEEDED(source->CreatePresentationDescriptor(pd.address()))) {
        UINT64 duration = 0;
        pd->GetUINT64(MF_PD_DURATION, &duration);
        duration /= 10000;
        m_duration = qint64(duration);
        durationChanged(m_duration);
    }

    if (useResampler()) {
        HRESULT hr = m_resampler->SetInputType(m_mfInputStreamID, mediaType.get(), 0);
        if (SUCCEEDED(hr)) {
            if (auto output = QWindowsAudioUtils::formatToMediaType(m_outputFormat); output) {
                hr = setResamplerOutputType(output.get());
                if (FAILED(hr)) {
                    qWarning() << "MFAudioDecoderControl: failed to SetOutputType of resampler: "
                               << std::system_category().message(hr).c_str();
                }
            } else {
                qWarning() << "MFAudioDecoderControl: incorrect audio format";
            }
        } else {
            qWarning() << "MFAudioDecoderControl: failed to SetInputType of resampler: "
                       << std::system_category().message(hr).c_str();
        }
    }

    connect(m_decoderSourceReader.get(), SIGNAL(finished()), this, SLOT(handleSourceFinished()));
    connect(m_decoderSourceReader.get(), SIGNAL(newSample(QWindowsIUPointer<IMFSample>)), this, SLOT(handleNewSample(QWindowsIUPointer<IMFSample>)));

    setIsDecoding(true);

    m_decoderSourceReader->readNextSample();
}

void MFAudioDecoderControl::start()
{
    if (isDecoding())
        return;

    if (m_loadingSource) {
        m_deferredStart = true;
    } else {
        IMFMediaSource *source = m_sourceResolver->mediaSource();
        if (!source) {
            if (m_device)
                error(QAudioDecoder::ResourceError, tr("Unable to read from specified device"));
            else if (m_source.isValid())
                error(QAudioDecoder::ResourceError, tr("Unable to load specified URL"));
            else
                error(QAudioDecoder::ResourceError, tr("No media source specified"));
            return;
        } else {
            startReadingSource(source);
        }
    }
}

void MFAudioDecoderControl::stop()
{
    m_deferredStart = false;
    if (!isDecoding())
        return;

    disconnect(m_decoderSourceReader.get());
    m_decoderSourceReader->clearSource();
    m_decoderSourceReader.reset();

    if (bufferAvailable()) {
        QAudioBuffer buffer;
        m_audioBuffer.swap(buffer);
        emit bufferAvailableChanged(false);
    }
    setIsDecoding(false);

    if (m_position != -1) {
        m_position = -1;
        positionChanged(m_position);
    }
    if (m_duration != -1) {
        m_duration = -1;
        durationChanged(m_duration);
    }
}

static HRESULT addDataFromIMFSample(QByteArray &data, IMFSample *s)
{
    Q_ASSERT(s);

    QWindowsIUPointer<IMFMediaBuffer> mediaBuffer;
    HRESULT hr = s->ConvertToContiguousBuffer(mediaBuffer.address());
    if (FAILED(hr))
        return hr;

    DWORD bufLen = 0;
    BYTE *buf = nullptr;
    hr = mediaBuffer->Lock(&buf, nullptr, &bufLen);
    if (SUCCEEDED(hr))
        data.push_back(QByteArray(reinterpret_cast<char *>(buf), bufLen));

    mediaBuffer->Unlock();

    return hr;
}

void MFAudioDecoderControl::handleNewSample(QWindowsIUPointer<IMFSample> sample)
{
    Q_ASSERT(sample);
    LONGLONG sampleStartTime;
    QByteArray abuf;

    HRESULT hr = S_OK;
    if (useResampler()) {
        hr = m_resampler->ProcessInput(m_mfInputStreamID, sample.get(), 0);
        if (SUCCEEDED(hr)) {
            bool getSampleStartTime = true;
            MFT_OUTPUT_DATA_BUFFER outputDataBuffer;
            outputDataBuffer.dwStreamID = m_mfOutputStreamID;
            do {
                outputDataBuffer.pEvents = nullptr;
                outputDataBuffer.dwStatus = 0;
                outputDataBuffer.pSample = m_convertSample.get();
                DWORD status = 0;
                hr = m_resampler->ProcessOutput(0, 1, &outputDataBuffer, &status);
                if (SUCCEEDED(hr)) {
                    if (getSampleStartTime) {
                        getSampleStartTime = false;
                        outputDataBuffer.pSample->GetSampleTime(&sampleStartTime);
                    }
                    hr = addDataFromIMFSample(abuf, outputDataBuffer.pSample);
                }
            } while (SUCCEEDED(hr));

            if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
                hr = S_OK;
        }
    } else {
        sample->GetSampleTime(&sampleStartTime);
        hr = addDataFromIMFSample(abuf, sample.get());
    }

    if (FAILED(hr)) {
        error(QAudioDecoder::Error::ResourceError, tr("Failed processing a sample")
              + QString::fromStdString(std::system_category().message(hr)));
        return;
    }
    // WMF uses 100-nanosecond units, QAudioDecoder uses milliseconds, QAudioBuffer uses microseconds...
    m_audioBuffer = QAudioBuffer(abuf, useResampler() ? m_outputFormat : m_mediaFormat, qint64(sampleStartTime / 10));

    emit bufferAvailableChanged(true);
    emit bufferReady();
}

void MFAudioDecoderControl::handleSourceFinished()
{
    stop();
    emit finished();
}

void MFAudioDecoderControl::setAudioFormat(const QAudioFormat &format)
{
    if (m_outputFormat == format || !m_resampler)
        return;
    m_outputFormat = format;
    emit formatChanged(m_outputFormat);
}

QAudioBuffer MFAudioDecoderControl::read()
{
    QAudioBuffer buffer;

    if (bufferAvailable()) {
        buffer.swap(m_audioBuffer);
        m_position = buffer.startTime() / 1000;
        emit positionChanged(m_position);
        emit bufferAvailableChanged(false);
        m_decoderSourceReader->readNextSample();
    }

    return buffer;
}
