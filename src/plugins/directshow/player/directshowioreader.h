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

#ifndef DIRECTSHOWIOREADER_H
#define DIRECTSHOWIOREADER_H

#include <dshow.h>

#include <QtCore/qmutex.h>
#include <QtCore/qobject.h>
#include <QtCore/qwaitcondition.h>

QT_BEGIN_NAMESPACE
class QIODevice;

class DirectShowEventLoop;
class DirectShowIOSource;
class DirectShowSampleRequest;

class DirectShowIOReader : public QObject, public IAsyncReader
{
    Q_OBJECT
public:
    DirectShowIOReader(QIODevice *device, DirectShowIOSource *source, DirectShowEventLoop *loop);
    ~DirectShowIOReader() override;

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    // IAsyncReader
    HRESULT STDMETHODCALLTYPE RequestAllocator(
            IMemAllocator *pPreferred, ALLOCATOR_PROPERTIES *pProps,
            IMemAllocator **ppActual) override;

    HRESULT STDMETHODCALLTYPE Request(IMediaSample *pSample, DWORD_PTR dwUser) override;

    HRESULT STDMETHODCALLTYPE WaitForNext(
            DWORD dwTimeout, IMediaSample **ppSample, DWORD_PTR *pdwUser) override;

    HRESULT STDMETHODCALLTYPE SyncReadAligned(IMediaSample *pSample) override;

    HRESULT STDMETHODCALLTYPE SyncRead(LONGLONG llPosition, LONG lLength, BYTE *pBuffer) override;

    HRESULT STDMETHODCALLTYPE Length(LONGLONG *pTotal, LONGLONG *pAvailable) override;

    HRESULT STDMETHODCALLTYPE BeginFlush() override;
    HRESULT STDMETHODCALLTYPE EndFlush() override;

protected:
    void customEvent(QEvent *event) override;

private Q_SLOTS:
    void readyRead();

private:
    HRESULT blockingRead(LONGLONG position, LONG length, BYTE *buffer, qint64 *bytesRead);
    bool nonBlockingRead(
            LONGLONG position, LONG length, BYTE *buffer, qint64 *bytesRead, HRESULT *result);
    void flushRequests();

    DirectShowIOSource *m_source;
    QIODevice *m_device;
    DirectShowEventLoop *m_loop;
    DirectShowSampleRequest *m_pendingHead = nullptr;
    DirectShowSampleRequest *m_pendingTail = nullptr;
    DirectShowSampleRequest *m_readyHead = nullptr;
    DirectShowSampleRequest *m_readyTail = nullptr;
    LONGLONG m_synchronousPosition = 0;
    LONG m_synchronousLength = 0;
    qint64 m_synchronousBytesRead = 0;
    BYTE *m_synchronousBuffer = nullptr;
    HRESULT m_synchronousResult = S_OK;
    LONGLONG m_totalLength = 0;
    LONGLONG m_availableLength = 0;
    bool m_flushing = false;
    QMutex m_mutex;
    QWaitCondition m_wait;
};

QT_END_NAMESPACE

#endif
