/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Mobility Components.
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

#ifndef MFSTREAM_H
#define MFSTREAM_H

#include <mfapi.h>
#include <mfidl.h>
#include <QtCore/qmutex.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qcoreevent.h>

QT_USE_NAMESPACE

class MFStream : public QObject, public IMFByteStream
{
    Q_OBJECT
public:
    MFStream(QIODevice *stream, bool ownStream);

    ~MFStream();

    //from IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObject);

    STDMETHODIMP_(ULONG) AddRef(void);

    STDMETHODIMP_(ULONG) Release(void);


    //from IMFByteStream
    STDMETHODIMP GetCapabilities(DWORD *pdwCapabilities);

    STDMETHODIMP GetLength(QWORD *pqwLength);

    STDMETHODIMP SetLength(QWORD);

    STDMETHODIMP GetCurrentPosition(QWORD *pqwPosition);

    STDMETHODIMP SetCurrentPosition(QWORD qwPosition);

    STDMETHODIMP IsEndOfStream(BOOL *pfEndOfStream);

    STDMETHODIMP Read(BYTE *pb, ULONG cb, ULONG *pcbRead);

    STDMETHODIMP BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback,
                           IUnknown *punkState);

    STDMETHODIMP EndRead(IMFAsyncResult* pResult, ULONG *pcbRead);

    STDMETHODIMP Write(const BYTE *, ULONG, ULONG *);

    STDMETHODIMP BeginWrite(const BYTE *, ULONG ,
                            IMFAsyncCallback *,
                            IUnknown *);

    STDMETHODIMP EndWrite(IMFAsyncResult *,
                          ULONG *);

    STDMETHODIMP Seek(
        MFBYTESTREAM_SEEK_ORIGIN SeekOrigin,
        LONGLONG llSeekOffset,
        DWORD,
        QWORD *pqwCurrentPosition);

    STDMETHODIMP Flush();

    STDMETHODIMP Close();

private:
    class AsyncReadState : public IUnknown
    {
    public:
        AsyncReadState(BYTE *pb, ULONG cb);

        //from IUnknown
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObject);

        STDMETHODIMP_(ULONG) AddRef(void);

        STDMETHODIMP_(ULONG) Release(void);

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
    QIODevice *m_stream;
    bool m_ownStream;
    DWORD m_workQueueId;
    QMutex m_mutex;

    void doRead();

private Q_SLOTS:
    void handleReadyRead();

protected:
    void customEvent(QEvent *event);
    IMFAsyncResult *m_currentReadResult;
};

#endif
