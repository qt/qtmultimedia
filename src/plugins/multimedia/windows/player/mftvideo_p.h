// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef MFTVIDEO_H
#define MFTVIDEO_H

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
#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtMultimedia/qvideoframeformat.h>


QT_BEGIN_NAMESPACE

class MFVideoProbeControl;

class QVideoFrame;

class MFTransform: public IMFTransform
{
public:
    MFTransform();
    virtual ~MFTransform();

    void addProbe(MFVideoProbeControl* probe);
    void removeProbe(MFVideoProbeControl* probe);

    void setVideoSink(IUnknown *videoSink);

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IMFTransform methods
    STDMETHODIMP GetStreamLimits(DWORD *pdwInputMinimum, DWORD *pdwInputMaximum, DWORD *pdwOutputMinimum, DWORD *pdwOutputMaximum) override;
    STDMETHODIMP GetStreamCount(DWORD *pcInputStreams, DWORD *pcOutputStreams) override;
    STDMETHODIMP GetStreamIDs(DWORD dwInputIDArraySize, DWORD *pdwInputIDs, DWORD dwOutputIDArraySize, DWORD *pdwOutputIDs) override;
    STDMETHODIMP GetInputStreamInfo(DWORD dwInputStreamID, MFT_INPUT_STREAM_INFO *pStreamInfo) override;
    STDMETHODIMP GetOutputStreamInfo(DWORD dwOutputStreamID, MFT_OUTPUT_STREAM_INFO *pStreamInfo) override;
    STDMETHODIMP GetAttributes(IMFAttributes **pAttributes) override;
    STDMETHODIMP GetInputStreamAttributes(DWORD dwInputStreamID, IMFAttributes **pAttributes) override;
    STDMETHODIMP GetOutputStreamAttributes(DWORD dwOutputStreamID, IMFAttributes **pAttributes) override;
    STDMETHODIMP DeleteInputStream(DWORD dwStreamID) override;
    STDMETHODIMP AddInputStreams(DWORD cStreams, DWORD *adwStreamIDs) override;
    STDMETHODIMP GetInputAvailableType(DWORD dwInputStreamID, DWORD dwTypeIndex, IMFMediaType **ppType) override;
    STDMETHODIMP GetOutputAvailableType(DWORD dwOutputStreamID,DWORD dwTypeIndex, IMFMediaType **ppType) override;
    STDMETHODIMP SetInputType(DWORD dwInputStreamID, IMFMediaType *pType, DWORD dwFlags) override;
    STDMETHODIMP SetOutputType(DWORD dwOutputStreamID, IMFMediaType *pType, DWORD dwFlags) override;
    STDMETHODIMP GetInputCurrentType(DWORD dwInputStreamID, IMFMediaType **ppType) override;
    STDMETHODIMP GetOutputCurrentType(DWORD dwOutputStreamID, IMFMediaType **ppType) override;
    STDMETHODIMP GetInputStatus(DWORD dwInputStreamID, DWORD *pdwFlags) override;
    STDMETHODIMP GetOutputStatus(DWORD *pdwFlags) override;
    STDMETHODIMP SetOutputBounds(LONGLONG hnsLowerBound, LONGLONG hnsUpperBound) override;
    STDMETHODIMP ProcessEvent(DWORD dwInputStreamID, IMFMediaEvent *pEvent) override;
    STDMETHODIMP ProcessMessage(MFT_MESSAGE_TYPE eMessage, ULONG_PTR ulParam) override;
    STDMETHODIMP ProcessInput(DWORD dwInputStreamID, IMFSample *pSample, DWORD dwFlags) override;
    STDMETHODIMP ProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount, MFT_OUTPUT_DATA_BUFFER *pOutputSamples, DWORD *pdwStatus) override;

private:
    HRESULT OnFlush();
    static QVideoFrameFormat videoFormatForMFMediaType(IMFMediaType *mediaType, int *bytesPerLine);
    QVideoFrame makeVideoFrame();
    QByteArray dataFromBuffer(IMFMediaBuffer *buffer, int height, int *bytesPerLine);
    bool isMediaTypeSupported(IMFMediaType *type);

    long m_cRef;
    IMFMediaType *m_inputType;
    IMFMediaType *m_outputType;
    IMFSample *m_sample;
    QMutex m_mutex;

    IMFMediaTypeHandler *m_videoSinkTypeHandler;

//    QList<MFVideoProbeControl*> m_videoProbes;
    QMutex m_videoProbeMutex;

    QVideoFrameFormat m_format;
    int m_bytesPerLine;
};

QT_END_NAMESPACE

#endif
