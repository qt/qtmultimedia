// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <system_error>
#include <mferror.h>
#include <qglobal.h>
#include "wmcodecdsp.h"
#include "mfaudiodecodercontrol_p.h"
#include <private/qwindowsaudioutils_p.h>

QT_BEGIN_NAMESPACE

MFAudioDecoderControl::MFAudioDecoderControl(QAudioDecoder *parent)
    : QPlatformAudioDecoder(parent)
    , m_sourceResolver(new SourceResolver)
{
    connect(m_sourceResolver, &SourceResolver::mediaSourceReady, this, &MFAudioDecoderControl::handleMediaSourceReady);
    connect(m_sourceResolver, &SourceResolver::error, this, &MFAudioDecoderControl::handleMediaSourceError);
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

    m_decoderSourceReader = makeComObject<MFDecoderSourceReader>();
    if (!m_decoderSourceReader) {
        error(QAudioDecoder::ResourceError, tr("Could not instantiate MFDecoderSourceReader"));
        return;
    }

    auto mediaType = m_decoderSourceReader->setSource(source, m_outputFormat.sampleFormat());
    QAudioFormat mediaFormat = QWindowsAudioUtils::mediaTypeToFormat(mediaType.Get());
    if (!mediaFormat.isValid()) {
        error(QAudioDecoder::FormatError, tr("Invalid media format"));
        m_decoderSourceReader.Reset();
        return;
    }

    ComPtr<IMFPresentationDescriptor> pd;
    if (SUCCEEDED(source->CreatePresentationDescriptor(pd.GetAddressOf()))) {
        UINT64 duration = 0;
        pd->GetUINT64(MF_PD_DURATION, &duration);
        duration /= 10000;
        m_duration = qint64(duration);
        durationChanged(m_duration);
    }

    if (!m_resampler.setup(mediaFormat, m_outputFormat.isValid() ? m_outputFormat : mediaFormat)) {
        qWarning() << "Failed to set up resampler";
        return;
    }

    connect(m_decoderSourceReader.Get(), &MFDecoderSourceReader::finished, this, &MFAudioDecoderControl::handleSourceFinished);
    connect(m_decoderSourceReader.Get(), &MFDecoderSourceReader::newSample, this, &MFAudioDecoderControl::handleNewSample);

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

    disconnect(m_decoderSourceReader.Get());
    m_decoderSourceReader->clearSource();
    m_decoderSourceReader.Reset();

    if (bufferAvailable()) {
        QAudioBuffer buffer;
        m_audioBuffer.swap(buffer);
        bufferAvailableChanged(false);
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

void MFAudioDecoderControl::handleNewSample(ComPtr<IMFSample> sample)
{
    Q_ASSERT(sample);

    qint64 sampleStartTimeUs = m_resampler.outputFormat().durationForBytes(m_resampler.totalOutputBytes());
    QByteArray out = m_resampler.resample(sample.Get());

    if (out.isEmpty()) {
        error(QAudioDecoder::Error::ResourceError, tr("Failed processing a sample"));

    } else {
        m_audioBuffer = QAudioBuffer(out, m_resampler.outputFormat(), sampleStartTimeUs);

        bufferAvailableChanged(true);
        bufferReady();
    }
}

void MFAudioDecoderControl::handleSourceFinished()
{
    stop();
    finished();
}

void MFAudioDecoderControl::setAudioFormat(const QAudioFormat &format)
{
    if (m_outputFormat == format)
        return;
    m_outputFormat = format;
    formatChanged(m_outputFormat);
}

QAudioBuffer MFAudioDecoderControl::read()
{
    QAudioBuffer buffer;

    if (bufferAvailable()) {
        buffer.swap(m_audioBuffer);
        m_position = buffer.startTime() / 1000;
        positionChanged(m_position);
        bufferAvailableChanged(false);
        m_decoderSourceReader->readNextSample();
    }

    return buffer;
}

QT_END_NAMESPACE

#include "moc_mfaudiodecodercontrol_p.cpp"
