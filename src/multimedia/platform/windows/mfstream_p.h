/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

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

QT_USE_NAMESPACE

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

private Q_SLOTS:
    void handleReadyRead();

protected:
    void customEvent(QEvent *event) override;
    IMFAsyncResult *m_currentReadResult;
};

#endif
