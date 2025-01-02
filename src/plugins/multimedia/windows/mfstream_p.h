// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef MFSTREAM_H
#define MFSTREAM_H

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
#include <QtCore/qmutex.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class MFStream : public QObject, public IMFByteStream
{
    Q_OBJECT
public:
    MFStream(QIODevice *stream, bool ownStream);

    ~MFStream();

    //from IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObject) override;

    STDMETHODIMP_(ULONG) AddRef(void) override;

    STDMETHODIMP_(ULONG) Release(void) override;


    //from IMFByteStream
    STDMETHODIMP GetCapabilities(DWORD *pdwCapabilities) override;

    STDMETHODIMP GetLength(QWORD *pqwLength) override;

    STDMETHODIMP SetLength(QWORD) override;

    STDMETHODIMP GetCurrentPosition(QWORD *pqwPosition) override;

    STDMETHODIMP SetCurrentPosition(QWORD qwPosition) override;

    STDMETHODIMP IsEndOfStream(BOOL *pfEndOfStream) override;

    STDMETHODIMP Read(BYTE *pb, ULONG cb, ULONG *pcbRead) override;

    STDMETHODIMP BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback,
                           IUnknown *punkState) override;

    STDMETHODIMP EndRead(IMFAsyncResult* pResult, ULONG *pcbRead) override;

    STDMETHODIMP Write(const BYTE *, ULONG, ULONG *) override;

    STDMETHODIMP BeginWrite(const BYTE *, ULONG ,
                            IMFAsyncCallback *,
                            IUnknown *) override;

    STDMETHODIMP EndWrite(IMFAsyncResult *,
                          ULONG *) override;

    STDMETHODIMP Seek(
        MFBYTESTREAM_SEEK_ORIGIN SeekOrigin,
        LONGLONG llSeekOffset,
        DWORD,
        QWORD *pqwCurrentPosition) override;

    STDMETHODIMP Flush() override;

    STDMETHODIMP Close() override;

private:
    class AsyncReadState : public IUnknown
    {
    public:
        AsyncReadState(BYTE *pb, ULONG cb);
        virtual ~AsyncReadState() = default;

        //from IUnknown
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObject) override;

        STDMETHODIMP_(ULONG) AddRef(void) override;

        STDMETHODIMP_(ULONG) Release(void) override;

        BYTE* pb() const;
        ULONG cb() const;
        ULONG bytesRead() const;

        void setBytesRead(ULONG cbRead);

    private:
        long m_cRef;
        BYTE *m_pb;
        ULONG m_cb;
        ULONG m_cbRead;
    };

    long m_cRef;
    QPointer<QIODevice> m_stream;
    bool m_ownStream;
    DWORD m_workQueueId;
    QMutex m_mutex;

    void doRead();

protected:
    void customEvent(QEvent *event) override;
    IMFAsyncResult *m_currentReadResult;
};

QT_END_NAMESPACE

#endif
