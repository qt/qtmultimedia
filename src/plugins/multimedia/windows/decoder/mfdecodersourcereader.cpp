// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <system_error>
#include <mfapi.h>
#include <mferror.h>
#include <qlogging.h>
#include <qdebug.h>
#include "mfdecodersourcereader_p.h"
#include "mfdecodersourcereadercallback_p.h"

QT_BEGIN_NAMESPACE

ComPtr<IMFMediaType> MFDecoderSourceReader::setSource(IMFMediaSource *source, QAudioFormat::SampleFormat sampleFormat)
{
    ComPtr<IMFMediaType> mediaType;
    m_sourceReader.Reset();

    if (m_sourceReaderCallback) {
        disconnect(m_sourceReaderCallback.Get());

        m_sourceReaderCallback.Reset();
    }

    if (!source)
        return mediaType;

    m_sourceReaderCallback = makeComObject<MFSourceReaderCallback>();

    connect(m_sourceReaderCallback.Get(), &MFSourceReaderCallback::finished, this,
            &MFDecoderSourceReader::finished);
    connect(m_sourceReaderCallback.Get(), &MFSourceReaderCallback::newSample, this,
            &MFDecoderSourceReader::newSample);

    ComPtr<IMFAttributes> attr;
    MFCreateAttributes(attr.GetAddressOf(), 1);
    if (FAILED(attr->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, m_sourceReaderCallback.Get())))
        return mediaType;
    if (FAILED(attr->SetUINT32(MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN, TRUE)))
        return mediaType;

    HRESULT hr = MFCreateSourceReaderFromMediaSource(source, attr.Get(), m_sourceReader.GetAddressOf());
    if (FAILED(hr)) {
        qWarning() << "MFDecoderSourceReader: failed to set up source reader: "
                   << std::system_category().message(hr).c_str();
        return mediaType;
    }

    m_sourceReader->SetStreamSelection(DWORD(MF_SOURCE_READER_ALL_STREAMS), FALSE);
    m_sourceReader->SetStreamSelection(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM), TRUE);

    ComPtr<IMFMediaType> pPartialType;
    MFCreateMediaType(pPartialType.GetAddressOf());
    pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    pPartialType->SetGUID(MF_MT_SUBTYPE, sampleFormat == QAudioFormat::Float ? MFAudioFormat_Float : MFAudioFormat_PCM);
    m_sourceReader->SetCurrentMediaType(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM), nullptr, pPartialType.Get());
    m_sourceReader->GetCurrentMediaType(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM), mediaType.GetAddressOf());
    // Ensure the stream is selected.
    m_sourceReader->SetStreamSelection(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM), TRUE);

    return mediaType;
}

void MFDecoderSourceReader::readNextSample()
{
    if (m_sourceReader)
        m_sourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, NULL, NULL, NULL, NULL);
}

QT_END_NAMESPACE

#include "moc_mfdecodersourcereader_p.cpp"
