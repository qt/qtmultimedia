// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <system_error>
#include <mferror.h>
#include <qlogging.h>
#include <qdebug.h>
#include "mfdecodersourcereader_p.h"

QT_BEGIN_NAMESPACE

ComPtr<IMFMediaType> MFDecoderSourceReader::setSource(IMFMediaSource *source, QAudioFormat::SampleFormat sampleFormat)
{
    ComPtr<IMFMediaType> mediaType;
    m_sourceReader.Reset();

    if (!source)
        return mediaType;

    ComPtr<IMFAttributes> attr;
    MFCreateAttributes(attr.GetAddressOf(), 1);
    if (FAILED(attr->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this)))
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

//from IUnknown
STDMETHODIMP MFDecoderSourceReader::QueryInterface(REFIID riid, LPVOID *ppvObject)
{
    if (!ppvObject)
        return E_POINTER;
    if (riid == IID_IMFSourceReaderCallback) {
        *ppvObject = static_cast<IMFSourceReaderCallback*>(this);
    } else if (riid == IID_IUnknown) {
        *ppvObject = static_cast<IUnknown*>(this);
    } else {
        *ppvObject =  NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) MFDecoderSourceReader::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) MFDecoderSourceReader::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        this->deleteLater();
    }
    return cRef;
}

//from IMFSourceReaderCallback
STDMETHODIMP MFDecoderSourceReader::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
    DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample)
{
    Q_UNUSED(hrStatus);
    Q_UNUSED(dwStreamIndex);
    Q_UNUSED(llTimestamp);
    if (pSample) {
        emit newSample(ComPtr<IMFSample>{pSample});
    } else if ((dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM) == MF_SOURCE_READERF_ENDOFSTREAM) {
        emit finished();
    }
    return S_OK;
}

QT_END_NAMESPACE

#include "moc_mfdecodersourcereader_p.cpp"
