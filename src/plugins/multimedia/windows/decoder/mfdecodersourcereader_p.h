// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef MFDECODERSOURCEREADER_H
#define MFDECODERSOURCEREADER_H

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

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <QtCore/qobject.h>
#include "qaudioformat.h"
#include <private/qcomptr_p.h>

QT_BEGIN_NAMESPACE

class MFDecoderSourceReader : public QObject, public IMFSourceReaderCallback
{
    Q_OBJECT
public:
    MFDecoderSourceReader() {}
    ~MFDecoderSourceReader() override {}

    void clearSource() { m_sourceReader.Reset(); }
    ComPtr<IMFMediaType> setSource(IMFMediaSource *source, QAudioFormat::SampleFormat);

    void readNextSample();

    //from IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    //from IMFSourceReaderCallback
    STDMETHODIMP OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
        DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample) override;
    STDMETHODIMP OnFlush(DWORD) override { return S_OK; }
    STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *) override { return S_OK; }

Q_SIGNALS:
    void newSample(ComPtr<IMFSample>);
    void finished();

private:
    long m_cRef = 1;
    ComPtr<IMFSourceReader> m_sourceReader;

};

QT_END_NAMESPACE

#endif//MFDECODERSOURCEREADER_H
