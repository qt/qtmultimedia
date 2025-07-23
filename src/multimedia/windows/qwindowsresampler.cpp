// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsresampler_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qsystemerror_p.h>
#include <QtMultimedia/private/qaudio_alignment_support_p.h>
#include <QtMultimedia/private/qwindowsaudioutils_p.h>
#include <QtMultimedia/private/qwmf_support_p.h>

#include <wmcodecdsp.h>
#include <mftransform.h>
#include <mferror.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcAudioResampler, "qt.multimedia.audioresampler");

namespace {

HRESULT replaceBuffer(const ComPtr<IMFSample> &sample, const ComPtr<IMFMediaBuffer> &buffer)
{
    HRESULT hr = sample->RemoveAllBuffers();
    if (FAILED(hr))
        return hr;

    return sample->AddBuffer(buffer.Get());
}

} // namespace

bool QWindowsResampler::isAvailable()
{
    return QWindowsMediaFoundation::instance();
}

QWindowsResampler::QWindowsResampler()
{
    CoCreateInstance(__uuidof(CResamplerMediaObject), nullptr, CLSCTX_INPROC_SERVER,
                     IID_PPV_ARGS(&m_resampler));
    if (m_resampler)
        m_resampler->AddInputStreams(1, &m_inputStreamID);

    for (ComPtr<IMFSample> &sample : { std::ref(m_inputSample), std::ref(m_outputSample) }) {
        HRESULT hr = m_wmf->mfCreateSample(sample.GetAddressOf());
        if (FAILED(hr)) {
            qCWarning(qLcAudioResampler) << "Failed to create sample for resampling:"
                                         << QSystemError::windowsComString(hr);
            m_resampler = nullptr;
            return;
        }
    }
}

QWindowsResampler::~QWindowsResampler() = default;

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

qsizetype QWindowsResampler::overAllocatedOutputBufferSize()
{
    auto expectedOutputSize = outputBufferSize(m_totalInputBytes) - m_totalOutputBytes;
    // we may have some rounding errors, so we over-allocate by 10ms
    expectedOutputSize += m_outputFormat.bytesForDuration(10000);
    expectedOutputSize = QtMultimediaPrivate::alignUp(expectedOutputSize, 1024);
    return expectedOutputSize;
}

template <typename Functor>
auto QWindowsResampler::processOutput(ComPtr<IMFMediaBuffer> buffer, Functor &&f)
        -> std::invoke_result_t<Functor, const ComPtr<IMFMediaBuffer> &>
{
    HRESULT hr = replaceBuffer(m_outputSample, buffer);
    if (FAILED(hr))
        return q23::unexpected{ hr };

    MFT_OUTPUT_DATA_BUFFER outputDataBuffer;
    outputDataBuffer.dwStreamID = 0;
    outputDataBuffer.pEvents = nullptr;
    outputDataBuffer.dwStatus = 0;
    outputDataBuffer.pSample = m_outputSample.Get();
    DWORD status = 0;
    hr = m_resampler->ProcessOutput(0, 1, &outputDataBuffer, &status);
    if (FAILED(hr))
        return q23::unexpected{ hr };

    return f(buffer);
}

q23::expected<QByteArray, HRESULT> QWindowsResampler::processOutput()
{
    using namespace QWMF;

    ComPtr<IMFMediaBuffer> buffer;
    HRESULT hr = QByteArrayMFMediaBuffer::CreateInstance(overAllocatedOutputBufferSize(),
                                                         buffer.GetAddressOf());
    if (FAILED(hr))
        return q23::unexpected{ hr };

    return processOutput(std::move(buffer), [&](const ComPtr<IMFMediaBuffer> &buffer) {
        return withLockedBuffer(buffer, [&](QSpan<BYTE> data, QSpan<BYTE> /*max*/) {
            auto *byteArrayBuffer = static_cast<QByteArrayMFMediaBuffer *>(buffer.Get());
            QByteArray out = byteArrayBuffer->takeByteArray();
            out.truncate(data.size());
            return out;
        });
    });
}

QByteArray QWindowsResampler::resample(QByteArray in)
{
    m_totalInputBytes += in.size();

    if (m_inputFormat == m_outputFormat) {
        m_totalOutputBytes += in.size();
        return in;
    }

    Q_ASSERT(m_resampler && m_wmf);

    // process input
    ComPtr<IMFMediaBuffer> buffer;
    // we always do out-of-place transformations, so read-only will prevent the buffer to detach()
    QWMF::QByteArrayMFMediaBuffer::CreateInstance(std::move(in), buffer.GetAddressOf(),
                                                  /*isReadOnly=*/true);

    HRESULT hr = replaceBuffer(m_inputSample, buffer);
    if (FAILED(hr))
        return {};

    hr = m_resampler->ProcessInput(m_inputStreamID, m_inputSample.Get(), 0);
    if (FAILED(hr)) {
        qCWarning(qLcAudioResampler)
                << "Failed to process input" << QSystemError::windowsComString(hr);
        return {};
    }

    // process output
    auto result = processOutput();
    if (result) {
        m_totalOutputBytes += result.value().size();
        return result.value();
    }

    qCWarning(qLcAudioResampler) << "Resampling failed"
                                 << QSystemError::windowsComString(result.error());
    return {};
}

QByteArray QWindowsResampler::resample(const QByteArrayView &in)
{
    return QWindowsResampler::resample(QByteArray(in));
}

QByteArray QWindowsResampler::resample(const ComPtr<IMFSample> &sample)
{
    using namespace QWMF;

    Q_ASSERT(sample);

    DWORD totalLength = 0;
    HRESULT hr = sample->GetTotalLength(&totalLength);
    if (FAILED(hr))
        return {};

    m_totalInputBytes += totalLength;

    if (m_inputFormat == m_outputFormat) {
        ComPtr<IMFMediaBuffer> outputBuffer;
        sample->ConvertToContiguousBuffer(outputBuffer.GetAddressOf());

        auto result = withLockedBuffer(outputBuffer, [&](QSpan<BYTE> data, QSpan<BYTE> /*max*/) {
            return QByteArray(data);
        });
        if (result) {
            m_totalOutputBytes += result.value().size();
            return result.value();
        }
        qCWarning(qLcAudioResampler) << "Failed to convert sample to contiguous buffer"
                                     << QSystemError::windowsComString(result.error());
        return {};
    }

    Q_ASSERT(m_resampler && m_wmf);

    // process input
    hr = m_resampler->ProcessInput(m_inputStreamID, sample.Get(), 0);
    if (FAILED(hr)) {
        qCWarning(qLcAudioResampler)
                << "Failed to process input sample" << QSystemError::windowsComString(hr);
        return {};
    }

    // process output
    auto result = processOutput();
    if (result) {
        m_totalOutputBytes += result.value().size();
        return result.value();
    }
    qCWarning(qLcAudioResampler) << "Resampling failed" << QSystemError::windowsComString(hr);
    return {};
}

QAudioBuffer QWindowsResampler::resample(const char *data, size_t size)
{
    quint64 elapsedBytesAtStart = m_totalOutputBytes;

    QByteArray resampled = resample(QSpan{
            reinterpret_cast<const std::byte *>(data),
            qsizetype(size),
    });

    if (resampled.isEmpty())
        return {};

    return QAudioBuffer{
        std::move(resampled),
        m_outputFormat,
        m_outputFormat.durationForBytes(elapsedBytesAtStart) + m_startTimeOffset.count(),
    };
}

std::pmr::vector<std::byte> QWindowsResampler::resample(QSpan<const std::byte> in,
                                                        std::pmr::memory_resource *mr)
{
    using namespace QWMF;

    m_totalInputBytes += in.size_bytes();
    if (m_inputFormat == m_outputFormat) {
        m_totalOutputBytes += in.size();
        return std::pmr::vector<std::byte>{ in.begin(), in.end(), mr };
    }

    // process input
    ComPtr<IMFMediaBuffer> inputBuffer;
    HRESULT hr = QPmrMediaBuffer::CreateInstance(in, mr, inputBuffer.GetAddressOf());
    if (FAILED(hr))
        return {};

    hr = replaceBuffer(m_inputSample, inputBuffer);
    if (FAILED(hr))
        return {};

    hr = m_resampler->ProcessInput(m_inputStreamID, m_inputSample.Get(), 0);
    if (FAILED(hr))
        return {};

    // process output
    ComPtr<IMFMediaBuffer> outputBuffer;
    hr = QPmrMediaBuffer::CreateInstance(overAllocatedOutputBufferSize(), mr,
                                         outputBuffer.GetAddressOf());
    if (FAILED(hr)) {
        qCWarning(qLcAudioResampler)
                << "Failed to create output buffer" << QSystemError::windowsComString(hr);
        return {};
    }

    auto result = processOutput(std::move(outputBuffer), [&](const ComPtr<IMFMediaBuffer> &buffer) {
        return withLockedBuffer(buffer, [&](QSpan<BYTE> data, QSpan<BYTE> /*max*/) {
            return std::pmr::vector<std::byte>{
                reinterpret_cast<std::byte *>(data.begin()),
                reinterpret_cast<std::byte *>(data.end()),
                mr,
            };
        });
    });

    if (!result) {
        qCWarning(qLcAudioResampler)
                << "Resampling failed" << QSystemError::windowsComString(result.error());
        return {};
    }
    m_totalOutputBytes += result->size();
    return result.value();
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
        qCWarning(qLcAudioResampler)
                << "Failed to set input type" << QSystemError::windowsComString(hr);
        return false;
    }

    hr = m_resampler->SetOutputType(0, mout.Get(), 0);
    if (FAILED(hr)) {
        qCWarning(qLcAudioResampler)
                << "Failed to set output type" << QSystemError::windowsComString(hr);
        return false;
    }

    MFT_OUTPUT_STREAM_INFO streamInfo;
    hr = m_resampler->GetOutputStreamInfo(0, &streamInfo);
    if (FAILED(hr)) {
        qCWarning(qLcAudioResampler)
                << "Could not obtain stream info" << QSystemError::windowsComString(hr);
        return false;
    }

    m_inputFormat = fin;
    m_outputFormat = fout;

    return true;
}

void QWindowsResampler::setStartTimeOffset(std::chrono::microseconds startTime)
{
    m_startTimeOffset = startTime;
}

QT_END_NAMESPACE
