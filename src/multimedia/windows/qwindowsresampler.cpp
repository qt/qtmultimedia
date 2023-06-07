// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsresampler_p.h"
#include <qwindowsaudioutils_p.h>
#include <qwindowsmediafoundation_p.h>
#include <qloggingcategory.h>
#include <QUuid>

#include <wmcodecdsp.h>
#include <mftransform.h>
#include <mferror.h>

QT_BEGIN_NAMESPACE

QUuid qIID_IMFTransform(0xbf94c121, 0x5b05, 0x4e6f, 0x80,0x00, 0xba,0x59,0x89,0x61,0x41,0x4d);
QUuid qCLSID_CResamplerMediaObject("f447b69e-1884-4a7e-8055-346f74d6edb3");

static Q_LOGGING_CATEGORY(qLcAudioResampler, "qt.multimedia.audioresampler")

QWindowsResampler::QWindowsResampler()
    : m_wmf(QWindowsMediaFoundation::instance())
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (hr == RPC_E_CHANGED_MODE)
        qWarning() << "Failed attempt to change apartment concurrency model";

    CoCreateInstance(qCLSID_CResamplerMediaObject, nullptr, CLSCTX_INPROC_SERVER,
                     qIID_IMFTransform, (LPVOID*)(m_resampler.GetAddressOf()));
    if (m_resampler)
        m_resampler->AddInputStreams(1, &m_inputStreamID);
}

QWindowsResampler::~QWindowsResampler()
{
    CoUninitialize();
};

quint64 QWindowsResampler::outputBufferSize(quint64 inputBufferSize) const
{
    if (m_inputFormat.isValid() && m_outputFormat.isValid())
        return m_outputFormat.bytesForDuration(m_inputFormat.durationForBytes(inputBufferSize));
    else
        return 0;
}

quint64 QWindowsResampler::inputBufferSize(quint64 outputBufferSize) const
{
    if (m_inputFormat.isValid() && m_outputFormat.isValid())
        return m_inputFormat.bytesForDuration(m_outputFormat.durationForBytes(outputBufferSize));
    else
        return 0;
}

HRESULT QWindowsResampler::processInput(const QByteArrayView &in)
{
    ComPtr<IMFSample> sample;
    HRESULT hr = m_wmf->mfCreateSample(sample.GetAddressOf());
    if (FAILED(hr))
        return hr;

    ComPtr<IMFMediaBuffer> buffer;
    hr = m_wmf->mfCreateMemoryBuffer(in.size(), buffer.GetAddressOf());
    if (FAILED(hr))
        return hr;

    BYTE *data = nullptr;
    DWORD maxLen = 0;
    DWORD currentLen = 0;
    hr = buffer->Lock(&data, &maxLen, &currentLen);
    if (FAILED(hr))
        return hr;

    memcpy(data, in.data(), in.size());

    hr = buffer->Unlock();
    if (FAILED(hr))
        return hr;

    hr = buffer->SetCurrentLength(in.size());
    if (FAILED(hr))
        return hr;

    hr = sample->AddBuffer(buffer.Get());
    if (FAILED(hr))
        return hr;

    return m_resampler->ProcessInput(m_inputStreamID, sample.Get(), 0);
}

HRESULT QWindowsResampler::processOutput(QByteArray &out)
{
    ComPtr<IMFSample> sample;
    ComPtr<IMFMediaBuffer> buffer;

    if (m_resamplerNeedsSampleBuffer) {
        HRESULT hr = m_wmf->mfCreateSample(sample.GetAddressOf());
        if (FAILED(hr))
            return hr;

        auto expectedOutputSize = outputBufferSize(m_totalInputBytes) - m_totalOutputBytes;
        hr = m_wmf->mfCreateMemoryBuffer(expectedOutputSize, buffer.GetAddressOf());
        if (FAILED(hr))
            return hr;

        hr = sample->AddBuffer(buffer.Get());
        if (FAILED(hr))
            return hr;
    }

    HRESULT hr = S_OK;

    MFT_OUTPUT_DATA_BUFFER outputDataBuffer;
    outputDataBuffer.dwStreamID = 0;
    do {
        outputDataBuffer.pEvents = nullptr;
        outputDataBuffer.dwStatus = 0;
        outputDataBuffer.pSample = m_resamplerNeedsSampleBuffer ? sample.Get() : nullptr;
        DWORD status = 0;
        hr = m_resampler->ProcessOutput(0, 1, &outputDataBuffer, &status);
        if (SUCCEEDED(hr)) {
            ComPtr<IMFMediaBuffer> outputBuffer;
            outputDataBuffer.pSample->ConvertToContiguousBuffer(outputBuffer.GetAddressOf());
            DWORD len = 0;
            BYTE *data = nullptr;
            hr = outputBuffer->Lock(&data, nullptr, &len);
            if (SUCCEEDED(hr))
                out.push_back(QByteArray(reinterpret_cast<char *>(data), len));
            outputBuffer->Unlock();
        }
    } while (SUCCEEDED(hr));

    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
        hr = S_OK;

    return hr;
}

QByteArray QWindowsResampler::resample(const QByteArrayView &in)
{
    m_totalInputBytes += in.size();

    if (m_inputFormat == m_outputFormat) {
        m_totalOutputBytes += in.size();
        return {in.data(), in.size()};

    } else {
        Q_ASSERT(m_resampler && m_wmf);

        QByteArray out;
        HRESULT hr = processInput(in);
        if (SUCCEEDED(hr))
            hr = processOutput(out);

        if (FAILED(hr))
            qCWarning(qLcAudioResampler) << "Resampling failed" << hr;

        m_totalOutputBytes += out.size();
        return out;
    }
}

QByteArray QWindowsResampler::resample(IMFSample *sample)
{
    Q_ASSERT(sample);

    DWORD totalLength = 0;
    HRESULT hr = sample->GetTotalLength(&totalLength);
    if (FAILED(hr))
        return {};

    m_totalInputBytes += totalLength;

    QByteArray out;

    if (m_inputFormat == m_outputFormat) {
        ComPtr<IMFMediaBuffer> outputBuffer;
        sample->ConvertToContiguousBuffer(outputBuffer.GetAddressOf());
        DWORD len = 0;
        BYTE *data = nullptr;
        hr = outputBuffer->Lock(&data, nullptr, &len);
        if (SUCCEEDED(hr))
            out.push_back(QByteArray(reinterpret_cast<char *>(data), len));
        outputBuffer->Unlock();

    } else {
        Q_ASSERT(m_resampler && m_wmf);

        hr = m_resampler->ProcessInput(m_inputStreamID, sample, 0);
        if (SUCCEEDED(hr))
            hr = processOutput(out);

        if (FAILED(hr))
            qCWarning(qLcAudioResampler) << "Resampling failed" << hr;
    }

    m_totalOutputBytes += out.size();

    return out;
}

bool QWindowsResampler::setup(const QAudioFormat &fin, const QAudioFormat &fout)
{
    qCDebug(qLcAudioResampler) << "Setup audio resampler" << fin << "->" << fout;

    m_totalInputBytes = 0;
    m_totalOutputBytes = 0;

    if (fin == fout) {
        qCDebug(qLcAudioResampler) << "Pass through mode";
        m_inputFormat = fin;
        m_outputFormat = fout;
        return true;
    }

    if (!m_resampler || !m_wmf)
        return false;

    ComPtr<IMFMediaType> min = QWindowsAudioUtils::formatToMediaType(*m_wmf, fin);
    ComPtr<IMFMediaType> mout = QWindowsAudioUtils::formatToMediaType(*m_wmf, fout);

    HRESULT hr = m_resampler->SetInputType(m_inputStreamID, min.Get(), 0);
    if (FAILED(hr)) {
        qCWarning(qLcAudioResampler) << "Failed to set input type" << hr;
        return false;
    }

    hr = m_resampler->SetOutputType(0, mout.Get(), 0);
    if (FAILED(hr)) {
        qCWarning(qLcAudioResampler) << "Failed to set output type" << hr;
        return false;
    }

    MFT_OUTPUT_STREAM_INFO streamInfo;
    hr = m_resampler->GetOutputStreamInfo(0, &streamInfo);
    if (FAILED(hr)) {
        qCWarning(qLcAudioResampler) << "Could not obtain stream info" << hr;
        return false;
    }

    m_resamplerNeedsSampleBuffer = (streamInfo.dwFlags
             & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES)) == 0;

    m_inputFormat = fin;
    m_outputFormat = fout;

    return true;
}

QT_END_NAMESPACE
