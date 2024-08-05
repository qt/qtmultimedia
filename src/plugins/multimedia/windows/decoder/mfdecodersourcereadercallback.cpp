// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "mfdecodersourcereadercallback_p.h"

QT_BEGIN_NAMESPACE

STDMETHODIMP MFSourceReaderCallback::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
                                                  DWORD dwStreamFlags, LONGLONG llTimestamp,
                                                  IMFSample *pSample)
{
    Q_UNUSED(hrStatus);
    Q_UNUSED(dwStreamIndex);
    Q_UNUSED(llTimestamp);
    if (pSample) {
        emit newSample(ComPtr<IMFSample>{ pSample });
    } else if ((dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM) == MF_SOURCE_READERF_ENDOFSTREAM) {
        emit finished();
    }
    return S_OK;
}

QT_END_NAMESPACE

#include "moc_mfdecodersourcereadercallback_p.cpp"
