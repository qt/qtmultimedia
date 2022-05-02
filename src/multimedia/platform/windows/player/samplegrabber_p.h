/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef SAMPLEGRABBER_H
#define SAMPLEGRABBER_H

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

#include <QtCore/qmutex.h>
#include <QtCore/qlist.h>
#include <QtMultimedia/qaudioformat.h>
#include <mfapi.h>
#include <mfidl.h>

class SampleGrabberCallback : public IMFSampleGrabberSinkCallback
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IMFClockStateSink methods
    STDMETHODIMP OnClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset) override;
    STDMETHODIMP OnClockStop(MFTIME hnsSystemTime) override;
    STDMETHODIMP OnClockPause(MFTIME hnsSystemTime) override;
    STDMETHODIMP OnClockRestart(MFTIME hnsSystemTime) override;
    STDMETHODIMP OnClockSetRate(MFTIME hnsSystemTime, float flRate) override;

    // IMFSampleGrabberSinkCallback methods
    STDMETHODIMP OnSetPresentationClock(IMFPresentationClock* pClock) override;
    STDMETHODIMP OnShutdown() override;

protected:
    SampleGrabberCallback() : m_cRef(1) {}

public:
    virtual ~SampleGrabberCallback() {}

private:
    long m_cRef;
};

class AudioSampleGrabberCallback: public SampleGrabberCallback {
public:
    void setFormat(const QAudioFormat& format);

    STDMETHODIMP OnProcessSample(REFGUID guidMajorMediaType, DWORD dwSampleFlags,
        LONGLONG llSampleTime, LONGLONG llSampleDuration, const BYTE * pSampleBuffer,
        DWORD dwSampleSize) override;

private:
//    QList<MFAudioProbeControl*> m_audioProbes;
    QMutex m_audioProbeMutex;
    QAudioFormat m_format;
};

#endif // SAMPLEGRABBER_H
