// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "mfstream_p.h"

#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE
//MFStream is added for supporting QIODevice type of media source.
//It is used to delegate invocations from media foundation(through IMFByteStream) to QIODevice.

MFStream::MFStream(QIODevice *stream, bool ownStream)
    : m_stream(stream)
    , m_ownStream(ownStream)
{
    //Move to the thread of the stream object
    //to make sure invocations on stream
    //are happened in the same thread of stream object
    this->moveToThread(stream->thread());
}

MFStream::~MFStream()
{
    if (m_ownStream)
        m_stream->deleteLater();
}

//from IMFByteStream
STDMETHODIMP MFStream::GetCapabilities(DWORD *pdwCapabilities)
{
    if (!pdwCapabilities)
        return E_INVALIDARG;
    *pdwCapabilities = MFBYTESTREAM_IS_READABLE;
    if (!m_stream->isSequential())
        *pdwCapabilities |= MFBYTESTREAM_IS_SEEKABLE;
    return S_OK;
}

STDMETHODIMP MFStream::GetLength(QWORD *pqwLength)
{
    if (!pqwLength)
        return E_INVALIDARG;
    QMutexLocker locker(&m_mutex);
    *pqwLength = QWORD(m_stream->size());
    return S_OK;
}

STDMETHODIMP MFStream::SetLength(QWORD)
{
    return E_NOTIMPL;
}

STDMETHODIMP MFStream::GetCurrentPosition(QWORD *pqwPosition)
{
    if (!pqwPosition)
        return E_INVALIDARG;
    QMutexLocker locker(&m_mutex);
    *pqwPosition = m_stream->pos();
    return S_OK;
}

STDMETHODIMP MFStream::SetCurrentPosition(QWORD qwPosition)
{
    QMutexLocker locker(&m_mutex);
    //SetCurrentPosition may happend during the BeginRead/EndRead pair,
    //refusing to execute SetCurrentPosition during that time seems to be
    //the simplest workable solution
    if (m_currentReadResult)
        return S_FALSE;

    bool seekOK = m_stream->seek(qint64(qwPosition));
    if (seekOK)
        return S_OK;
    else
        return S_FALSE;
}

STDMETHODIMP MFStream::IsEndOfStream(BOOL *pfEndOfStream)
{
    if (!pfEndOfStream)
        return E_INVALIDARG;
    QMutexLocker locker(&m_mutex);
    *pfEndOfStream = m_stream->atEnd() ? TRUE : FALSE;
    return S_OK;
}

STDMETHODIMP MFStream::Read(BYTE *pb, ULONG cb, ULONG *pcbRead)
{
    QMutexLocker locker(&m_mutex);
    qint64 read = m_stream->read((char*)(pb), qint64(cb));
    if (pcbRead)
        *pcbRead = ULONG(read);
    return S_OK;
}

STDMETHODIMP MFStream::BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback,
                       IUnknown *punkState)
{
    if (!pCallback || !pb)
        return E_INVALIDARG;

    Q_ASSERT(m_currentReadResult == nullptr);

    ComPtr<AsyncReadState> state = makeComObject<AsyncReadState>(pb, cb);

    HRESULT hr = MFCreateAsyncResult(state.Get(), pCallback, punkState, &m_currentReadResult);
    if (FAILED(hr))
        return hr;

    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
    return hr;
}

STDMETHODIMP MFStream::EndRead(IMFAsyncResult* pResult, ULONG *pcbRead)
{
    if (!pcbRead)
        return E_INVALIDARG;

    ComPtr<IUnknown> pUnk;
    pResult->GetObject(&pUnk);
    auto *state = static_cast<AsyncReadState *>(pUnk.Get());
    *pcbRead = state->bytesRead();

    m_currentReadResult = nullptr;

    return S_OK;
}

STDMETHODIMP MFStream::Write(const BYTE *, ULONG, ULONG *)
{
    return E_NOTIMPL;
}

STDMETHODIMP MFStream::BeginWrite(const BYTE *, ULONG ,
                        IMFAsyncCallback *,
                        IUnknown *)
{
    return E_NOTIMPL;
}

STDMETHODIMP MFStream::EndWrite(IMFAsyncResult *,
                      ULONG *)
{
    return E_NOTIMPL;
}

STDMETHODIMP MFStream::Seek(
    MFBYTESTREAM_SEEK_ORIGIN SeekOrigin,
    LONGLONG llSeekOffset,
    DWORD,
    QWORD *pqwCurrentPosition)
{
    QMutexLocker locker(&m_mutex);
    if (m_currentReadResult)
        return S_FALSE;

    qint64 pos = qint64(llSeekOffset);
    switch (SeekOrigin) {
    case msoBegin:
        break;
    case msoCurrent:
        pos += m_stream->pos();
        break;
    }
    bool seekOK = m_stream->seek(pos);
    if (pqwCurrentPosition)
        *pqwCurrentPosition = pos;
    if (seekOK)
        return S_OK;
    else
        return S_FALSE;
}

STDMETHODIMP MFStream::Flush()
{
    return E_NOTIMPL;
}

STDMETHODIMP MFStream::Close()
{
    QMutexLocker locker(&m_mutex);
    if (m_ownStream)
        m_stream->close();
    return S_OK;
}

void MFStream::doRead()
{
    if (!m_stream)
        return;

    bool readDone = true;
    ComPtr<IUnknown> pUnk;
    HRESULT hr = m_currentReadResult->GetObject(&pUnk);
    if (SUCCEEDED(hr)) {
        //do actual read
        auto *state = static_cast<AsyncReadState *>(pUnk.Get());
        ULONG cbRead;
        Read(state->pb(), state->cb() - state->bytesRead(), &cbRead);

        state->setBytesRead(cbRead + state->bytesRead());
        if (state->cb() > state->bytesRead() && !m_stream->atEnd()) {
            readDone = false;
        }
    }

    if (readDone) {
        //now inform the original caller
        m_currentReadResult->SetStatus(hr);
        MFInvokeCallback(m_currentReadResult.Get());
    }
}

void MFStream::customEvent(QEvent *event)
{
    if (event->type() != QEvent::User) {
        QObject::customEvent(event);
        return;
    }
    doRead();
}

//AsyncReadState is a helper class used in BeginRead for asynchronous operation
//to record some BeginRead parameters, so these parameters could be
//used later when actually executing the read operation in another thread.
MFStream::AsyncReadState::AsyncReadState(BYTE *pb, ULONG cb)
    : m_pb(pb)
    , m_cb(cb)
    , m_cbRead(0)
{
}


BYTE* MFStream::AsyncReadState::pb() const
{
    return m_pb;
}

ULONG MFStream::AsyncReadState::cb() const
{
    return m_cb;
}

ULONG MFStream::AsyncReadState::bytesRead() const
{
    return m_cbRead;
}

void MFStream::AsyncReadState::setBytesRead(ULONG cbRead)
{
    m_cbRead = cbRead;
}

QT_END_NAMESPACE

#include "moc_mfstream_p.cpp"
