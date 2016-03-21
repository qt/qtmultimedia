/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef DIRECTSHOWSAMPLESCHEDULER_H
#define DIRECTSHOWSAMPLESCHEDULER_H

#include <dshow.h>

#include <QtCore/qmutex.h>
#include <QtCore/qobject.h>
#include <QtCore/qsemaphore.h>

class DirectShowTimedSample;

class DirectShowSampleScheduler : public QObject, public IMemInputPin
{
    Q_OBJECT
public:

    enum State
    {
        Stopped  = 0x00,
        Running  = 0x01,
        Paused   = 0x02,
        RunMask  = 0x03,
        Flushing = 0x04
    };

    DirectShowSampleScheduler(IUnknown *pin, QObject *parent = 0);
    ~DirectShowSampleScheduler();

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IMemInputPin
    HRESULT STDMETHODCALLTYPE GetAllocator(IMemAllocator **ppAllocator);
    HRESULT STDMETHODCALLTYPE NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly);
    HRESULT STDMETHODCALLTYPE GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps);

    HRESULT STDMETHODCALLTYPE Receive(IMediaSample *pSample);
    HRESULT STDMETHODCALLTYPE ReceiveMultiple(IMediaSample **pSamples, long nSamples, long *nSamplesProcessed);
    HRESULT STDMETHODCALLTYPE ReceiveCanBlock();

    void run(REFERENCE_TIME startTime);
    void pause();
    void stop();
    void setFlushing(bool flushing);

    IReferenceClock *clock() const { return m_clock; }
    void setClock(IReferenceClock *clock);

    bool schedule(IMediaSample *sample);
    bool scheduleEndOfStream();

    IMediaSample *takeSample(bool *eos);

    bool event(QEvent *event);

Q_SIGNALS:
    void sampleReady();

private:
    IUnknown *m_pin;
    IReferenceClock *m_clock;
    IMemAllocator *m_allocator;
    DirectShowTimedSample *m_head;
    DirectShowTimedSample *m_tail;
    int m_maximumSamples;
    int m_state;
    REFERENCE_TIME m_startTime;
    HANDLE m_timeoutEvent;
    HANDLE m_flushEvent;
    QSemaphore m_semaphore;
    QMutex m_mutex;
};

#endif
