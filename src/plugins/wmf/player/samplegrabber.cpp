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

#include "samplegrabber.h"
#include "mfaudioprobecontrol.h"

STDMETHODIMP SampleGrabberCallback::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;
    if (riid == IID_IMFSampleGrabberSinkCallback) {
        *ppv = static_cast<IMFSampleGrabberSinkCallback*>(this);
    } else if (riid == IID_IMFClockStateSink) {
        *ppv = static_cast<IMFClockStateSink*>(this);
    } else if (riid == IID_IUnknown) {
        *ppv = static_cast<IUnknown*>(this);
    } else {
        *ppv =  NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) SampleGrabberCallback::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) SampleGrabberCallback::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        delete this;
    }
    return cRef;

}

// IMFClockStateSink methods.

STDMETHODIMP SampleGrabberCallback::OnClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset)
{
    Q_UNUSED(hnsSystemTime);
    Q_UNUSED(llClockStartOffset);
    return S_OK;
}

STDMETHODIMP SampleGrabberCallback::OnClockStop(MFTIME hnsSystemTime)
{
    Q_UNUSED(hnsSystemTime);
    return S_OK;
}

STDMETHODIMP SampleGrabberCallback::OnClockPause(MFTIME hnsSystemTime)
{
    Q_UNUSED(hnsSystemTime);
    return S_OK;
}

STDMETHODIMP SampleGrabberCallback::OnClockRestart(MFTIME hnsSystemTime)
{
    Q_UNUSED(hnsSystemTime);
    return S_OK;
}

STDMETHODIMP SampleGrabberCallback::OnClockSetRate(MFTIME hnsSystemTime, float flRate)
{
    Q_UNUSED(hnsSystemTime);
    Q_UNUSED(flRate);
    return S_OK;
}

// IMFSampleGrabberSink methods.

STDMETHODIMP SampleGrabberCallback::OnSetPresentationClock(IMFPresentationClock* pClock)
{
    Q_UNUSED(pClock);
    return S_OK;
}

STDMETHODIMP SampleGrabberCallback::OnShutdown()
{
    return S_OK;
}

void AudioSampleGrabberCallback::addProbe(MFAudioProbeControl* probe)
{
    QMutexLocker locker(&m_audioProbeMutex);

    if (m_audioProbes.contains(probe))
        return;

    m_audioProbes.append(probe);
}

void AudioSampleGrabberCallback::removeProbe(MFAudioProbeControl* probe)
{
    QMutexLocker locker(&m_audioProbeMutex);
    m_audioProbes.removeOne(probe);
}

void AudioSampleGrabberCallback::setFormat(const QAudioFormat& format)
{
    m_format = format;
}

STDMETHODIMP AudioSampleGrabberCallback::OnProcessSample(REFGUID guidMajorMediaType, DWORD dwSampleFlags,
    LONGLONG llSampleTime, LONGLONG llSampleDuration, const BYTE * pSampleBuffer,
    DWORD dwSampleSize)
{
    Q_UNUSED(dwSampleFlags);
    Q_UNUSED(llSampleTime);
    Q_UNUSED(llSampleDuration);

    if (guidMajorMediaType != GUID_NULL && guidMajorMediaType != MFMediaType_Audio)
        return S_OK;

    QMutexLocker locker(&m_audioProbeMutex);

    if (m_audioProbes.isEmpty())
        return S_OK;

    // Check if sample has a presentation time
    if (llSampleTime == _I64_MAX) {
        // Set default QAudioBuffer start time
        llSampleTime = -1;
    } else {
        // WMF uses 100-nanosecond units, Qt uses microseconds
        llSampleTime /= 10;
    }

    for (MFAudioProbeControl* probe : qAsConst(m_audioProbes))
        probe->bufferProbed((const char*)pSampleBuffer, dwSampleSize, m_format, llSampleTime);

    return S_OK;
}
