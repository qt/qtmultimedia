// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "evrcustompresenter_p.h"

#include "evrd3dpresentengine_p.h"
#include "evrhelpers_p.h"
#include <private/qwindowsmultimediautils_p.h>
#include <private/qplatformvideosink_p.h>

#include <rhi/qrhi.h>

#include <QtCore/qmutex.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qrect.h>
#include <qthread.h>
#include <qcoreapplication.h>
#include <qmath.h>
#include <qloggingcategory.h>

#include <mutex>

#include <float.h>
#include <evcode.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcEvrCustomPresenter, "qt.multimedia.evrcustompresenter");

const static MFRatio g_DefaultFrameRate = { 30, 1 };
static const DWORD SCHEDULER_TIMEOUT = 5000;
static const MFTIME ONE_SECOND = 10000000;
static const LONG   ONE_MSEC = 1000;

#define QMM_PRESENTATION_CURRENT_POSITION 0x7fffffffffffffff

// Function declarations.
static HRESULT setMixerSourceRect(IMFTransform *mixer, const MFVideoNormalizedRect& nrcSource);
static QVideoFrameFormat::PixelFormat pixelFormatFromMediaType(IMFMediaType *type);

static inline LONG MFTimeToMsec(const LONGLONG& time)
{
    return (LONG)(time / (ONE_SECOND / ONE_MSEC));
}

bool qt_evr_setCustomPresenter(IUnknown *evr, EVRCustomPresenter *presenter)
{
    if (!evr || !presenter)
        return false;

    HRESULT result = E_FAIL;

    IMFVideoRenderer *renderer = NULL;
    if (SUCCEEDED(evr->QueryInterface(IID_PPV_ARGS(&renderer)))) {
        result = renderer->InitializeRenderer(NULL, presenter);
        renderer->Release();
    }

    return result == S_OK;
}

class PresentSampleEvent : public QEvent
{
public:
    explicit PresentSampleEvent(const ComPtr<IMFSample> &sample)
        : QEvent(static_cast<Type>(EVRCustomPresenter::PresentSample)), m_sample(sample)
    {
    }

    ComPtr<IMFSample> sample() const { return m_sample; }

private:
    const ComPtr<IMFSample> m_sample;
};

Scheduler::Scheduler(EVRCustomPresenter *presenter)
    : m_presenter(presenter)
    , m_threadID(0)
    , m_playbackRate(1.0f)
    , m_perFrame_1_4th(0)
{
}

Scheduler::~Scheduler()
{
    m_scheduledSamples.clear();
}

void Scheduler::setFrameRate(const MFRatio& fps)
{
    UINT64 AvgTimePerFrame = 0;

    // Convert to a duration.
    MFFrameRateToAverageTimePerFrame(fps.Numerator, fps.Denominator, &AvgTimePerFrame);

    // Calculate 1/4th of this value, because we use it frequently.
    m_perFrame_1_4th = AvgTimePerFrame / 4;
}

HRESULT Scheduler::startScheduler(ComPtr<IMFClock> clock)
{
    if (m_schedulerThread)
        return E_UNEXPECTED;

    HRESULT hr = S_OK;
    DWORD dwID = 0;
    HANDLE hObjects[2];
    DWORD dwWait = 0;

    m_clock = clock;

    // Set a high the timer resolution (ie, short timer period).
    timeBeginPeriod(1);

    // Create an event to wait for the thread to start.
    m_threadReadyEvent = EventHandle{ CreateEvent(NULL, FALSE, FALSE, NULL) };
    if (!m_threadReadyEvent) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    // Create an event to wait for flush commands to complete.
    m_flushEvent = EventHandle{ CreateEvent(NULL, FALSE, FALSE, NULL) };
    if (!m_flushEvent) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    // Create the scheduler thread.
    m_schedulerThread = ThreadHandle{ CreateThread(NULL, 0, schedulerThreadProc, (LPVOID)this, 0, &dwID) };
    if (!m_schedulerThread) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    // Wait for the thread to signal the "thread ready" event.
    hObjects[0] = m_threadReadyEvent.get();
    hObjects[1] = m_schedulerThread.get();
    dwWait = WaitForMultipleObjects(2, hObjects, FALSE, INFINITE);  // Wait for EITHER of these handles.
    if (WAIT_OBJECT_0 != dwWait) {
        // The thread terminated early for some reason. This is an error condition.
        m_schedulerThread = {};

        hr = E_UNEXPECTED;
        goto done;
    }

    m_threadID = dwID;

done:
    // Regardless success/failure, we are done using the "thread ready" event.
    m_threadReadyEvent = {};

    return hr;
}

HRESULT Scheduler::stopScheduler()
{
    if (!m_schedulerThread)
        return S_OK;

    // Ask the scheduler thread to exit.
    PostThreadMessage(m_threadID, Terminate, 0, 0);

    // Wait for the thread to exit.
    WaitForSingleObject(m_schedulerThread.get(), INFINITE);

    // Close handles.
    m_schedulerThread = {};
    m_flushEvent = {};

    // Discard samples.
    m_mutex.lock();
    m_scheduledSamples.clear();
    m_mutex.unlock();

    // Restore the timer resolution.
    timeEndPeriod(1);

    return S_OK;
}

HRESULT Scheduler::flush()
{
    if (m_schedulerThread) {
        // Ask the scheduler thread to flush.
        PostThreadMessage(m_threadID, Flush, 0 , 0);

        // Wait for the scheduler thread to signal the flush event,
        // OR for the thread to terminate.
        HANDLE objects[] = { m_flushEvent.get(), m_schedulerThread.get() };

        WaitForMultipleObjects(ARRAYSIZE(objects), objects, FALSE, SCHEDULER_TIMEOUT);
    }

    return S_OK;
}

bool Scheduler::areSamplesScheduled()
{
    QMutexLocker locker(&m_mutex);
    return m_scheduledSamples.count() > 0;
}

HRESULT Scheduler::scheduleSample(const ComPtr<IMFSample> &sample, bool presentNow)
{
    if (!m_schedulerThread)
        return MF_E_NOT_INITIALIZED;

    HRESULT hr = S_OK;
    DWORD dwExitCode = 0;

    GetExitCodeThread(m_schedulerThread.get(), &dwExitCode);
    if (dwExitCode != STILL_ACTIVE)
        return E_FAIL;

    if (presentNow || !m_clock) {
        m_presenter->presentSample(sample);
    } else {
        if (m_playbackRate > 0.0f && qt_evr_isSampleTimePassed(m_clock.Get(), sample.Get())) {
            qCDebug(qLcEvrCustomPresenter) << "Discard the sample, it came too late";
            return hr;
        }

        // Queue the sample and ask the scheduler thread to wake up.
        m_mutex.lock();
        m_scheduledSamples.enqueue(sample);
        m_mutex.unlock();

        if (SUCCEEDED(hr))
            PostThreadMessage(m_threadID, Schedule, 0, 0);
    }

    return hr;
}

HRESULT Scheduler::processSamplesInQueue(LONG *nextSleep)
{
    HRESULT hr = S_OK;
    LONG wait = 0;

    QQueue<ComPtr<IMFSample>> scheduledSamples;

    m_mutex.lock();
    m_scheduledSamples.swap(scheduledSamples);
    m_mutex.unlock();

    // Process samples until the queue is empty or until the wait time > 0.
    while (!scheduledSamples.isEmpty()) {
        ComPtr<IMFSample> sample = scheduledSamples.dequeue();

        // Process the next sample in the queue. If the sample is not ready
        // for presentation. the value returned in wait is > 0, which
        // means the scheduler should sleep for that amount of time.
        if (isSampleReadyToPresent(sample.Get(), &wait)) {
            m_presenter->presentSample(sample.Get());
            continue;
        }

        if (wait > 0) {
            // return the sample to scheduler
            scheduledSamples.prepend(sample);
            break;
        }
    }

    m_mutex.lock();
    scheduledSamples.append(std::move(m_scheduledSamples));
    m_scheduledSamples.swap(scheduledSamples);
    m_mutex.unlock();

    // If the wait time is zero, it means we stopped because the queue is
    // empty (or an error occurred). Set the wait time to infinite; this will
    // make the scheduler thread sleep until it gets another thread message.
    if (wait == 0)
        wait = INFINITE;

    *nextSleep = wait;
    return hr;
}

bool Scheduler::isSampleReadyToPresent(IMFSample *sample, LONG *pNextSleep) const
{
    *pNextSleep = 0;
    if (!m_clock)
        return true;

    MFTIME hnsPresentationTime = 0;
    MFTIME hnsTimeNow = 0;
    MFTIME hnsSystemTime = 0;

    // Get the sample's time stamp. It is valid for a sample to
    // have no time stamp.
    HRESULT hr = sample->GetSampleTime(&hnsPresentationTime);

    // Get the clock time. (But if the sample does not have a time stamp,
    // we don't need the clock time.)
    if (SUCCEEDED(hr))
        hr = m_clock->GetCorrelatedTime(0, &hnsTimeNow, &hnsSystemTime);

    // Calculate the time until the sample's presentation time.
    // A negative value means the sample is late.
    MFTIME hnsDelta = hnsPresentationTime - hnsTimeNow;
    if (m_playbackRate < 0) {
        // For reverse playback, the clock runs backward. Therefore, the
        // delta is reversed.
        hnsDelta = - hnsDelta;
    }

    if (hnsDelta < - m_perFrame_1_4th) {
        // This sample is late - skip.
        return false;
    } else if (hnsDelta > (3 * m_perFrame_1_4th)) {
        // This sample came too early - reschedule
        *pNextSleep = MFTimeToMsec(hnsDelta - (3 * m_perFrame_1_4th));

        // Adjust the sleep time for the clock rate. (The presentation clock runs
        // at m_fRate, but sleeping uses the system clock.)
        if (m_playbackRate != 0)
            *pNextSleep = (LONG)(*pNextSleep / qFabs(m_playbackRate));
        return *pNextSleep == 0;
    } else {
        // This sample can be presented right now
        return true;
    }
}

DWORD WINAPI Scheduler::schedulerThreadProc(LPVOID parameter)
{
    Scheduler* scheduler = reinterpret_cast<Scheduler*>(parameter);
    if (!scheduler)
        return -1;
    return scheduler->schedulerThreadProcPrivate();
}

DWORD Scheduler::schedulerThreadProcPrivate()
{
    HRESULT hr = S_OK;
    MSG msg;
    LONG wait = INFINITE;
    bool exitThread = false;

    // Force the system to create a message queue for this thread.
    // (See MSDN documentation for PostThreadMessage.)
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    // Signal to the scheduler that the thread is ready.
    SetEvent(m_threadReadyEvent.get());

    while (!exitThread) {
        // Wait for a thread message OR until the wait time expires.
        DWORD result = MsgWaitForMultipleObjects(0, NULL, FALSE, wait, QS_POSTMESSAGE);

        if (result == WAIT_TIMEOUT) {
            // If we timed out, then process the samples in the queue
            hr = processSamplesInQueue(&wait);
            if (FAILED(hr))
                exitThread = true;
        }

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            bool processSamples = true;

            switch (msg.message) {
            case Terminate:
                exitThread = true;
                break;
            case Flush:
                // Flushing: Clear the sample queue and set the event.
                m_mutex.lock();
                m_scheduledSamples.clear();
                m_mutex.unlock();
                wait = INFINITE;
                SetEvent(m_flushEvent.get());
                break;
            case Schedule:
                // Process as many samples as we can.
                if (processSamples) {
                    hr = processSamplesInQueue(&wait);
                    if (FAILED(hr))
                        exitThread = true;
                    processSamples = (wait != (LONG)INFINITE);
                }
                break;
            }
        }

    }

    return (SUCCEEDED(hr) ? 0 : 1);
}


SamplePool::SamplePool()
    : m_initialized(false)
{
}

SamplePool::~SamplePool()
{
    clear();
}

ComPtr<IMFSample> SamplePool::takeSample()
{
    QMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        qCWarning(qLcEvrCustomPresenter) << "SamplePool is not initialized yet";
        return nullptr;
    }

    if (m_videoSampleQueue.isEmpty()) {
        qCDebug(qLcEvrCustomPresenter) << "SamplePool is empty";
        return nullptr;
    }

    // Get a sample from the allocated queue.

    // It doesn't matter if we pull them from the head or tail of the list,
    // but when we get it back, we want to re-insert it onto the opposite end.
    // (see returnSample)

    return m_videoSampleQueue.takeFirst();
}

void SamplePool::returnSample(const ComPtr<IMFSample> &sample)
{
    QMutexLocker locker(&m_mutex);

    Q_ASSERT(m_initialized);
    if (!m_initialized) {
        qCWarning(qLcEvrCustomPresenter) << "SamplePool is not initialized yet";
        return;
    }

    m_videoSampleQueue.append(sample);
}

HRESULT SamplePool::initialize(QList<ComPtr<IMFSample>> &&samples)
{
    QMutexLocker locker(&m_mutex);

    if (m_initialized)
        return MF_E_INVALIDREQUEST;

    // Move these samples into our allocated queue.
    m_videoSampleQueue.append(std::move(samples));

    m_initialized = true;

    return S_OK;
}

HRESULT SamplePool::clear()
{
    QMutexLocker locker(&m_mutex);

    m_videoSampleQueue.clear();
    m_initialized = false;

    return S_OK;
}


EVRCustomPresenter::EVRCustomPresenter(QVideoSink *sink)
    : QObject()
    , m_sampleFreeCB(this, &EVRCustomPresenter::onSampleFree)
    , m_refCount(1)
    , m_renderState(RenderShutdown)
    , m_scheduler(this)
    , m_tokenCounter(0)
    , m_sampleNotify(false)
    , m_prerolled(false)
    , m_endStreaming(false)
    , m_playbackRate(1.0f)
    , m_presentEngine(new D3DPresentEngine(sink))
    , m_mediaType(0)
    , m_videoSink(0)
    , m_canRenderToSurface(false)
    , m_positionOffset(0)
{
    // Initial source rectangle = (0,0,1,1)
    m_sourceRect.top = 0;
    m_sourceRect.left = 0;
    m_sourceRect.bottom = 1;
    m_sourceRect.right = 1;

    setSink(sink);
}

EVRCustomPresenter::~EVRCustomPresenter()
{
    m_scheduler.flush();
    m_scheduler.stopScheduler();
    m_samplePool.clear();

    delete m_presentEngine;
}

HRESULT EVRCustomPresenter::QueryInterface(REFIID riid, void ** ppvObject)
{
    if (!ppvObject)
        return E_POINTER;
    if (riid == IID_IMFGetService) {
        *ppvObject = static_cast<IMFGetService*>(this);
    } else if (riid == IID_IMFTopologyServiceLookupClient) {
        *ppvObject = static_cast<IMFTopologyServiceLookupClient*>(this);
    } else if (riid == IID_IMFVideoDeviceID) {
        *ppvObject = static_cast<IMFVideoDeviceID*>(this);
    } else if (riid == IID_IMFVideoPresenter) {
        *ppvObject = static_cast<IMFVideoPresenter*>(this);
    } else if (riid == IID_IMFRateSupport) {
        *ppvObject = static_cast<IMFRateSupport*>(this);
    } else if (riid == IID_IUnknown) {
        *ppvObject = static_cast<IUnknown*>(static_cast<IMFGetService*>(this));
    } else if (riid == IID_IMFClockStateSink) {
        *ppvObject = static_cast<IMFClockStateSink*>(this);
    } else {
        *ppvObject =  NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG EVRCustomPresenter::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

ULONG EVRCustomPresenter::Release()
{
    ULONG uCount = InterlockedDecrement(&m_refCount);
    if (uCount == 0)
        deleteLater();
    return uCount;
}

HRESULT EVRCustomPresenter::GetService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
    HRESULT hr = S_OK;

    if (!ppvObject)
        return E_POINTER;

    // The only service GUID that we support is MR_VIDEO_RENDER_SERVICE.
    if (guidService != MR_VIDEO_RENDER_SERVICE)
        return MF_E_UNSUPPORTED_SERVICE;

    // First try to get the service interface from the D3DPresentEngine object.
    hr = m_presentEngine->getService(guidService, riid, ppvObject);
    if (FAILED(hr))
        // Next, check if this object supports the interface.
        hr = QueryInterface(riid, ppvObject);

    return hr;
}

HRESULT EVRCustomPresenter::GetDeviceID(IID* deviceID)
{
    if (!deviceID)
        return E_POINTER;

    *deviceID = IID_IDirect3DDevice9;

    return S_OK;
}

HRESULT EVRCustomPresenter::InitServicePointers(IMFTopologyServiceLookup *lookup)
{
    if (!lookup)
        return E_POINTER;

    HRESULT hr = S_OK;
    DWORD objectCount = 0;

    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    // Do not allow initializing when playing or paused.
    if (isActive())
        return MF_E_INVALIDREQUEST;

    m_clock.Reset();
    m_mixer.Reset();
    m_mediaEventSink.Reset();

    // Ask for the clock. Optional, because the EVR might not have a clock.
    objectCount = 1;

    lookup->LookupService(MF_SERVICE_LOOKUP_GLOBAL, 0,
                          MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&m_clock),
                          &objectCount
                          );

    // Ask for the mixer. (Required.)
    objectCount = 1;

    hr = lookup->LookupService(MF_SERVICE_LOOKUP_GLOBAL, 0,
                               MR_VIDEO_MIXER_SERVICE, IID_PPV_ARGS(&m_mixer),
                               &objectCount
                               );

    if (FAILED(hr))
        return hr;

    // Make sure that we can work with this mixer.
    hr = configureMixer(m_mixer.Get());
    if (FAILED(hr))
        return hr;

    // Ask for the EVR's event-sink interface. (Required.)
    objectCount = 1;

    hr = lookup->LookupService(MF_SERVICE_LOOKUP_GLOBAL, 0,
                               MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&m_mediaEventSink),
                               &objectCount
                               );

    if (SUCCEEDED(hr))
        m_renderState = RenderStopped;

    return hr;
}

HRESULT EVRCustomPresenter::ReleaseServicePointers()
{
    // Enter the shut-down state.
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    m_renderState = RenderShutdown;

    // Flush any samples that were scheduled.
    flush();

    // Clear the media type and release related resources.
    setMediaType(NULL);

    // Release all services that were acquired from InitServicePointers.
    m_clock.Reset();
    m_mixer.Reset();
    m_mediaEventSink.Reset();

    return S_OK;
}

bool EVRCustomPresenter::isValid() const
{
    return m_presentEngine->isValid() && m_canRenderToSurface;
}

HRESULT EVRCustomPresenter::ProcessMessage(MFVP_MESSAGE_TYPE message, ULONG_PTR param)
{
    HRESULT hr = S_OK;

    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    hr = checkShutdown();
    if (FAILED(hr))
        return hr;

    switch (message) {
    // Flush all pending samples.
    case MFVP_MESSAGE_FLUSH:
        hr = flush();
        break;

    // Renegotiate the media type with the mixer.
    case MFVP_MESSAGE_INVALIDATEMEDIATYPE:
        hr = renegotiateMediaType();
        break;

    // The mixer received a new input sample.
    case MFVP_MESSAGE_PROCESSINPUTNOTIFY:
        hr = processInputNotify();
        break;

    // Streaming is about to start.
    case MFVP_MESSAGE_BEGINSTREAMING:
        hr = beginStreaming();
        break;

    // Streaming has ended. (The EVR has stopped.)
    case MFVP_MESSAGE_ENDSTREAMING:
        hr = endStreaming();
        break;

    // All input streams have ended.
    case MFVP_MESSAGE_ENDOFSTREAM:
        // Set the EOS flag.
        m_endStreaming = true;
        // Check if it's time to send the EC_COMPLETE event to the EVR.
        hr = checkEndOfStream();
        break;

    // Frame-stepping is starting.
    case MFVP_MESSAGE_STEP:
        hr = prepareFrameStep(DWORD(param));
        break;

    // Cancels frame-stepping.
    case MFVP_MESSAGE_CANCELSTEP:
        hr = cancelFrameStep();
        break;

    default:
        hr = E_INVALIDARG; // Unknown message. This case should never occur.
        break;
    }

    return hr;
}

HRESULT EVRCustomPresenter::GetCurrentMediaType(IMFVideoMediaType **mediaType)
{
    HRESULT hr = S_OK;

    if (!mediaType)
        return E_POINTER;

    *mediaType = NULL;

    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    hr = checkShutdown();
    if (FAILED(hr))
        return hr;

    if (!m_mediaType)
        return MF_E_NOT_INITIALIZED;

    return m_mediaType->QueryInterface(IID_PPV_ARGS(mediaType));
}

HRESULT EVRCustomPresenter::OnClockStart(MFTIME, LONGLONG clockStartOffset)
{
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    // We cannot start after shutdown.
    HRESULT hr = checkShutdown();
    if (FAILED(hr))
        return hr;

    // Check if the clock is already active (not stopped).
    if (isActive()) {
        m_renderState = RenderStarted;

        // If the clock position changes while the clock is active, it
        // is a seek request. We need to flush all pending samples.
        if (clockStartOffset != QMM_PRESENTATION_CURRENT_POSITION)
            flush();
    } else {
        m_renderState = RenderStarted;

        // The clock has started from the stopped state.

        // Possibly we are in the middle of frame-stepping OR have samples waiting
        // in the frame-step queue. Deal with these two cases first:
        hr = startFrameStep();
        if (FAILED(hr))
            return hr;
    }

    // Now try to get new output samples from the mixer.
    processOutputLoop();

    return hr;
}

HRESULT EVRCustomPresenter::OnClockRestart(MFTIME)
{
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    HRESULT hr = checkShutdown();
    if (FAILED(hr))
        return hr;

    // The EVR calls OnClockRestart only while paused.

    m_renderState = RenderStarted;

    // Possibly we are in the middle of frame-stepping OR we have samples waiting
    // in the frame-step queue. Deal with these two cases first:
    hr = startFrameStep();
    if (FAILED(hr))
        return hr;

    // Now resume the presentation loop.
    processOutputLoop();

    return hr;
}

HRESULT EVRCustomPresenter::OnClockStop(MFTIME)
{
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    HRESULT hr = checkShutdown();
    if (FAILED(hr))
        return hr;

    if (m_renderState != RenderStopped) {
        m_renderState = RenderStopped;
        flush();

        // If we are in the middle of frame-stepping, cancel it now.
        if (m_frameStep.state != FrameStepNone)
            cancelFrameStep();
    }

    return S_OK;
}

HRESULT EVRCustomPresenter::OnClockPause(MFTIME)
{
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    // We cannot pause the clock after shutdown.
    HRESULT hr = checkShutdown();

    if (SUCCEEDED(hr))
        m_renderState = RenderPaused;

    return hr;
}

HRESULT EVRCustomPresenter::OnClockSetRate(MFTIME, float rate)
{
    // Note:
    // The presenter reports its maximum rate through the IMFRateSupport interface.
    // Here, we assume that the EVR honors the maximum rate.

    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    HRESULT hr = checkShutdown();
    if (FAILED(hr))
        return hr;

    // If the rate is changing from zero (scrubbing) to non-zero, cancel the
    // frame-step operation.
    if ((m_playbackRate == 0.0f) && (rate != 0.0f)) {
        cancelFrameStep();
        m_frameStep.samples.clear();
    }

    m_playbackRate = rate;

    // Tell the scheduler about the new rate.
    m_scheduler.setClockRate(rate);

    return S_OK;
}

HRESULT EVRCustomPresenter::GetSlowestRate(MFRATE_DIRECTION, BOOL, float *rate)
{
    if (!rate)
        return E_POINTER;

    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    HRESULT hr = checkShutdown();

    if (SUCCEEDED(hr)) {
        // There is no minimum playback rate, so the minimum is zero.
        *rate = 0;
    }

    return S_OK;
}

HRESULT EVRCustomPresenter::GetFastestRate(MFRATE_DIRECTION direction, BOOL thin, float *rate)
{
    if (!rate)
        return E_POINTER;

    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    float maxRate = 0.0f;

    HRESULT hr = checkShutdown();
    if (FAILED(hr))
        return hr;

    // Get the maximum *forward* rate.
    maxRate = getMaxRate(thin);

    // For reverse playback, it's the negative of maxRate.
    if (direction == MFRATE_REVERSE)
        maxRate = -maxRate;

    *rate = maxRate;

    return S_OK;
}

HRESULT EVRCustomPresenter::IsRateSupported(BOOL thin, float rate, float *nearestSupportedRate)
{
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    float maxRate = 0.0f;
    float nearestRate = rate;  // If we support rate, that is the nearest.

    HRESULT hr = checkShutdown();
    if (FAILED(hr))
        return hr;

    // Find the maximum forward rate.
    // Note: We have no minimum rate (that is, we support anything down to 0).
    maxRate = getMaxRate(thin);

    if (qFabs(rate) > maxRate) {
        // The (absolute) requested rate exceeds the maximum rate.
        hr = MF_E_UNSUPPORTED_RATE;

        // The nearest supported rate is maxRate.
        nearestRate = maxRate;
        if (rate < 0) {
            // Negative for reverse playback.
            nearestRate = -nearestRate;
        }
    }

    // Return the nearest supported rate.
    if (nearestSupportedRate)
        *nearestSupportedRate = nearestRate;

    return hr;
}

void EVRCustomPresenter::supportedFormatsChanged()
{
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    m_canRenderToSurface = false;

    // check if we can render to the surface (compatible formats)
    if (m_videoSink) {
        for (int f = 0; f < QVideoFrameFormat::NPixelFormats; ++f) {
            // ### set a better preference order
            QVideoFrameFormat::PixelFormat format = QVideoFrameFormat::PixelFormat(f);
            if (SUCCEEDED(m_presentEngine->checkFormat(qt_evr_D3DFormatFromPixelFormat(format)))) {
                m_canRenderToSurface = true;
                break;
            }
        }
    }

    // TODO: if media type already set, renegotiate?
}

void EVRCustomPresenter::setSink(QVideoSink *sink)
{
    m_mutex.lock();
    m_videoSink = sink;
    m_presentEngine->setSink(sink);
    m_mutex.unlock();

    supportedFormatsChanged();
}

void EVRCustomPresenter::setCropRect(QRect cropRect)
{
    m_mutex.lock();
    m_cropRect = cropRect;
    m_mutex.unlock();
}

HRESULT EVRCustomPresenter::configureMixer(IMFTransform *mixer)
{
    // Set the zoom rectangle (ie, the source clipping rectangle).
    return setMixerSourceRect(mixer, m_sourceRect);
}

HRESULT EVRCustomPresenter::renegotiateMediaType()
{
    HRESULT hr = S_OK;
    bool foundMediaType = false;

    IMFMediaType *mixerType = NULL;
    IMFMediaType *optimalType = NULL;

    if (!m_mixer)
        return MF_E_INVALIDREQUEST;

    // Loop through all of the mixer's proposed output types.
    DWORD typeIndex = 0;
    while (!foundMediaType && (hr != MF_E_NO_MORE_TYPES)) {
        qt_evr_safe_release(&mixerType);
        qt_evr_safe_release(&optimalType);

        // Step 1. Get the next media type supported by mixer.
        hr = m_mixer->GetOutputAvailableType(0, typeIndex++, &mixerType);
        if (FAILED(hr))
            break;

        // From now on, if anything in this loop fails, try the next type,
        // until we succeed or the mixer runs out of types.

        // Step 2. Check if we support this media type.
        if (SUCCEEDED(hr))
            hr = isMediaTypeSupported(mixerType);

        // Step 3. Adjust the mixer's type to match our requirements.
        if (SUCCEEDED(hr))
            hr = createOptimalVideoType(mixerType, &optimalType);

        // Step 4. Check if the mixer will accept this media type.
        if (SUCCEEDED(hr))
            hr = m_mixer->SetOutputType(0, optimalType, MFT_SET_TYPE_TEST_ONLY);

        // Step 5. Try to set the media type on ourselves.
        if (SUCCEEDED(hr))
            hr = setMediaType(optimalType);

        // Step 6. Set output media type on mixer.
        if (SUCCEEDED(hr)) {
            hr = m_mixer->SetOutputType(0, optimalType, 0);

            // If something went wrong, clear the media type.
            if (FAILED(hr))
                setMediaType(NULL);
        }

        if (SUCCEEDED(hr))
            foundMediaType = true;
    }

    qt_evr_safe_release(&mixerType);
    qt_evr_safe_release(&optimalType);

    return hr;
}

HRESULT EVRCustomPresenter::flush()
{
    m_prerolled = false;

    // The scheduler might have samples that are waiting for
    // their presentation time. Tell the scheduler to flush.

    // This call blocks until the scheduler threads discards all scheduled samples.
    m_scheduler.flush();

    // Flush the frame-step queue.
    m_frameStep.samples.clear();

    if (m_renderState == RenderStopped && m_videoSink) {
        // Repaint with black.
        presentSample(nullptr);
    }

    return S_OK;
}

HRESULT EVRCustomPresenter::processInputNotify()
{
    HRESULT hr = S_OK;

    // Set the flag that says the mixer has a new sample.
    m_sampleNotify = true;

    if (!m_mediaType) {
        // We don't have a valid media type yet.
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
    } else {
        // Try to process an output sample.
        processOutputLoop();
    }
    return hr;
}

HRESULT EVRCustomPresenter::beginStreaming()
{
    HRESULT hr = S_OK;

    // Start the scheduler thread.
    hr = m_scheduler.startScheduler(m_clock);

    return hr;
}

HRESULT EVRCustomPresenter::endStreaming()
{
    HRESULT hr = S_OK;

    // Stop the scheduler thread.
    hr = m_scheduler.stopScheduler();

    return hr;
}

HRESULT EVRCustomPresenter::checkEndOfStream()
{
    if (!m_endStreaming) {
        // The EVR did not send the MFVP_MESSAGE_ENDOFSTREAM message.
        return S_OK;
    }

    if (m_sampleNotify) {
        // The mixer still has input.
        return S_OK;
    }

    if (m_scheduler.areSamplesScheduled()) {
        // Samples are still scheduled for rendering.
        return S_OK;
    }

    // Everything is complete. Now we can tell the EVR that we are done.
    notifyEvent(EC_COMPLETE, (LONG_PTR)S_OK, 0);
    m_endStreaming = false;

    stopSurface();
    return S_OK;
}

HRESULT EVRCustomPresenter::prepareFrameStep(DWORD steps)
{
    HRESULT hr = S_OK;

    // Cache the step count.
    m_frameStep.steps += steps;

    // Set the frame-step state.
    m_frameStep.state = FrameStepWaitingStart;

    // If the clock is are already running, we can start frame-stepping now.
    // Otherwise, we will start when the clock starts.
    if (m_renderState == RenderStarted)
        hr = startFrameStep();

    return hr;
}

HRESULT EVRCustomPresenter::startFrameStep()
{
    if (m_frameStep.state == FrameStepWaitingStart) {
        // We have a frame-step request, and are waiting for the clock to start.
        // Set the state to "pending," which means we are waiting for samples.
        m_frameStep.state = FrameStepPending;

        // If the frame-step queue already has samples, process them now.
        while (!m_frameStep.samples.isEmpty() && (m_frameStep.state == FrameStepPending)) {
            const ComPtr<IMFSample> sample = m_frameStep.samples.takeFirst();

            const HRESULT hr = deliverFrameStepSample(sample.Get());
            if (FAILED(hr))
                return hr;

            // We break from this loop when:
            //   (a) the frame-step queue is empty, or
            //   (b) the frame-step operation is complete.
        }
    } else if (m_frameStep.state == FrameStepNone) {
        // We are not frame stepping. Therefore, if the frame-step queue has samples,
        // we need to process them normally.
        while (!m_frameStep.samples.isEmpty()) {
            const ComPtr<IMFSample> sample = m_frameStep.samples.takeFirst();

            const HRESULT hr = deliverSample(sample.Get());
            if (FAILED(hr))
                return hr;
        }
    }

    return S_OK;
}

HRESULT EVRCustomPresenter::completeFrameStep(const ComPtr<IMFSample> &sample)
{
    HRESULT hr = S_OK;
    MFTIME sampleTime = 0;
    MFTIME systemTime = 0;

    // Update our state.
    m_frameStep.state = FrameStepComplete;
    m_frameStep.sampleNoRef = 0;

    // Notify the EVR that the frame-step is complete.
    notifyEvent(EC_STEP_COMPLETE, FALSE, 0); // FALSE = completed (not cancelled)

    // If we are scrubbing (rate == 0), also send the "scrub time" event.
    if (isScrubbing()) {
        // Get the time stamp from the sample.
        hr = sample->GetSampleTime(&sampleTime);
        if (FAILED(hr)) {
            // No time stamp. Use the current presentation time.
            if (m_clock)
                m_clock->GetCorrelatedTime(0, &sampleTime, &systemTime);

            hr = S_OK; // (Not an error condition.)
        }

        notifyEvent(EC_SCRUB_TIME, DWORD(sampleTime), DWORD(((sampleTime) >> 32) & 0xffffffff));
    }
    return hr;
}

HRESULT EVRCustomPresenter::cancelFrameStep()
{
    FrameStepState oldState = m_frameStep.state;

    m_frameStep.state = FrameStepNone;
    m_frameStep.steps = 0;
    m_frameStep.sampleNoRef = 0;
    // Don't clear the frame-step queue yet, because we might frame step again.

    if (oldState > FrameStepNone && oldState < FrameStepComplete) {
        // We were in the middle of frame-stepping when it was cancelled.
        // Notify the EVR.
        notifyEvent(EC_STEP_COMPLETE, TRUE, 0); // TRUE = cancelled
    }
    return S_OK;
}

HRESULT EVRCustomPresenter::createOptimalVideoType(IMFMediaType *proposedType, IMFMediaType **optimalType)
{
    HRESULT hr = S_OK;

    RECT rcOutput;
    ZeroMemory(&rcOutput, sizeof(rcOutput));

    MFVideoArea displayArea;
    ZeroMemory(&displayArea, sizeof(displayArea));

    IMFMediaType *mtOptimal = NULL;

    UINT64 size;
    int width;
    int height;

    // Clone the proposed type.

    hr = MFCreateMediaType(&mtOptimal);
    if (FAILED(hr))
        goto done;

    hr = proposedType->CopyAllItems(mtOptimal);
    if (FAILED(hr))
        goto done;

    // Modify the new type.

    hr = proposedType->GetUINT64(MF_MT_FRAME_SIZE, &size);
    width = int(HI32(size));
    height = int(LO32(size));

    if (m_cropRect.isValid()) {
        rcOutput.left = m_cropRect.x();
        rcOutput.top = m_cropRect.y();
        rcOutput.right = m_cropRect.x() + m_cropRect.width();
        rcOutput.bottom = m_cropRect.y() + m_cropRect.height();

        m_sourceRect.left = float(m_cropRect.x()) / width;
        m_sourceRect.top = float(m_cropRect.y()) / height;
        m_sourceRect.right = float(m_cropRect.x() + m_cropRect.width()) / width;
        m_sourceRect.bottom = float(m_cropRect.y() + m_cropRect.height()) / height;

        if (m_mixer)
            configureMixer(m_mixer.Get());
    } else {
        rcOutput.left = 0;
        rcOutput.top = 0;
        rcOutput.right = width;
        rcOutput.bottom = height;
    }

    // Set the geometric aperture, and disable pan/scan.
    displayArea = qt_evr_makeMFArea(0, 0, rcOutput.right - rcOutput.left,
                                    rcOutput.bottom - rcOutput.top);

    hr = mtOptimal->SetUINT32(MF_MT_PAN_SCAN_ENABLED, FALSE);
    if (FAILED(hr))
        goto done;

    hr = mtOptimal->SetBlob(MF_MT_GEOMETRIC_APERTURE, reinterpret_cast<UINT8*>(&displayArea),
                            sizeof(displayArea));
    if (FAILED(hr))
        goto done;

    // Set the pan/scan aperture and the minimum display aperture. We don't care
    // about them per se, but the mixer will reject the type if these exceed the
    // frame dimentions.
    hr = mtOptimal->SetBlob(MF_MT_PAN_SCAN_APERTURE, reinterpret_cast<UINT8*>(&displayArea),
                            sizeof(displayArea));
    if (FAILED(hr))
        goto done;

    hr = mtOptimal->SetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, reinterpret_cast<UINT8*>(&displayArea),
                            sizeof(displayArea));
    if (FAILED(hr))
        goto done;

    // Return the pointer to the caller.
    *optimalType = mtOptimal;
    (*optimalType)->AddRef();

done:
    qt_evr_safe_release(&mtOptimal);
    return hr;

}

HRESULT EVRCustomPresenter::setMediaType(IMFMediaType *mediaType)
{
    // Note: mediaType can be NULL (to clear the type)

    // Clearing the media type is allowed in any state (including shutdown).
    if (!mediaType) {
        stopSurface();
        m_mediaType.Reset();
        releaseResources();
        return S_OK;
    }

    MFRatio fps = { 0, 0 };
    QList<ComPtr<IMFSample>> sampleQueue;

    // Cannot set the media type after shutdown.
    HRESULT hr = checkShutdown();
    if (FAILED(hr))
        goto done;

    // Check if the new type is actually different.
    // Note: This function safely handles NULL input parameters.
    if (qt_evr_areMediaTypesEqual(m_mediaType.Get(), mediaType))
        goto done; // Nothing more to do.

    // We're really changing the type. First get rid of the old type.
    m_mediaType.Reset();
    releaseResources();

    // Initialize the presenter engine with the new media type.
    // The presenter engine allocates the samples.

    hr = m_presentEngine->createVideoSamples(mediaType, sampleQueue, m_cropRect.size());
    if (FAILED(hr))
        goto done;

    // Mark each sample with our token counter. If this batch of samples becomes
    // invalid, we increment the counter, so that we know they should be discarded.
    for (auto sample : std::as_const(sampleQueue)) {
        hr = sample->SetUINT32(MFSamplePresenter_SampleCounter, m_tokenCounter);
        if (FAILED(hr))
            goto done;
    }

    // Add the samples to the sample pool.
    hr = m_samplePool.initialize(std::move(sampleQueue));
    if (FAILED(hr))
        goto done;

    // Set the frame rate on the scheduler.
    if (SUCCEEDED(qt_evr_getFrameRate(mediaType, &fps)) && (fps.Numerator != 0) && (fps.Denominator != 0)) {
        m_scheduler.setFrameRate(fps);
    } else {
        // NOTE: The mixer's proposed type might not have a frame rate, in which case
        // we'll use an arbitrary default. (Although it's unlikely the video source
        // does not have a frame rate.)
        m_scheduler.setFrameRate(g_DefaultFrameRate);
    }

    // Store the media type.
    m_mediaType = mediaType;
    m_mediaType->AddRef();

    startSurface();

done:
    if (FAILED(hr))
        releaseResources();
    return hr;
}

HRESULT EVRCustomPresenter::isMediaTypeSupported(IMFMediaType *proposed)
{
    D3DFORMAT d3dFormat = D3DFMT_UNKNOWN;
    BOOL compressed = FALSE;
    MFVideoInterlaceMode interlaceMode = MFVideoInterlace_Unknown;
    MFVideoArea videoCropArea;
    UINT32 width = 0, height = 0;

    // Validate the format.
    HRESULT hr = qt_evr_getFourCC(proposed, reinterpret_cast<DWORD*>(&d3dFormat));
    if (FAILED(hr))
        return hr;

    QVideoFrameFormat::PixelFormat pixelFormat = pixelFormatFromMediaType(proposed);
    if (pixelFormat == QVideoFrameFormat::Format_Invalid)
        return MF_E_INVALIDMEDIATYPE;

    // Reject compressed media types.
    hr = proposed->IsCompressedFormat(&compressed);
    if (FAILED(hr))
        return hr;

    if (compressed)
        return MF_E_INVALIDMEDIATYPE;

    // The D3DPresentEngine checks whether surfaces can be created using this format
    hr = m_presentEngine->checkFormat(d3dFormat);
    if (FAILED(hr))
        return hr;

    // Reject interlaced formats.
    hr = proposed->GetUINT32(MF_MT_INTERLACE_MODE, reinterpret_cast<UINT32*>(&interlaceMode));
    if (FAILED(hr))
        return hr;

    if (interlaceMode != MFVideoInterlace_Progressive)
        return MF_E_INVALIDMEDIATYPE;

    hr = MFGetAttributeSize(proposed, MF_MT_FRAME_SIZE, &width, &height);
    if (FAILED(hr))
        return hr;

    // Validate the various apertures (cropping regions) against the frame size.
    // Any of these apertures may be unspecified in the media type, in which case
    // we ignore it. We just want to reject invalid apertures.

    if (SUCCEEDED(proposed->GetBlob(MF_MT_PAN_SCAN_APERTURE,
                                    reinterpret_cast<UINT8*>(&videoCropArea),
                                    sizeof(videoCropArea), nullptr))) {
        hr = qt_evr_validateVideoArea(videoCropArea, width, height);
    }
    if (SUCCEEDED(proposed->GetBlob(MF_MT_GEOMETRIC_APERTURE,
                                    reinterpret_cast<UINT8*>(&videoCropArea),
                                    sizeof(videoCropArea), nullptr))) {
        hr = qt_evr_validateVideoArea(videoCropArea, width, height);
    }
    if (SUCCEEDED(proposed->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE,
                                    reinterpret_cast<UINT8*>(&videoCropArea),
                                    sizeof(videoCropArea), nullptr))) {
        hr = qt_evr_validateVideoArea(videoCropArea, width, height);
    }
    return hr;
}

void EVRCustomPresenter::processOutputLoop()
{
    HRESULT hr = S_OK;

    // Process as many samples as possible.
    while (hr == S_OK) {
        // If the mixer doesn't have a new input sample, break from the loop.
        if (!m_sampleNotify) {
            hr = MF_E_TRANSFORM_NEED_MORE_INPUT;
            break;
        }

        // Try to process a sample.
        hr = processOutput();

        // NOTE: ProcessOutput can return S_FALSE to indicate it did not
        // process a sample. If so, break out of the loop.
    }

    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
        // The mixer has run out of input data. Check for end-of-stream.
        checkEndOfStream();
    }
}

HRESULT EVRCustomPresenter::processOutput()
{
    // If the clock is not running, we present the first sample,
    // and then don't present any more until the clock starts.
    if ((m_renderState != RenderStarted) && m_prerolled)
        return S_FALSE;

    // Make sure we have a pointer to the mixer.
    if (!m_mixer)
        return MF_E_INVALIDREQUEST;

    // Try to get a free sample from the video sample pool.
    ComPtr<IMFSample> sample = m_samplePool.takeSample();
    if (!sample)
        return S_FALSE; // No free samples. Try again when a sample is released.

    // From now on, we have a valid video sample pointer, where the mixer will
    // write the video data.

    LONGLONG mixerStartTime = 0, mixerEndTime = 0;
    MFTIME systemTime = 0;

    if (m_clock) {
        // Latency: Record the starting time for ProcessOutput.
        m_clock->GetCorrelatedTime(0, &mixerStartTime, &systemTime);
    }

    // Now we are ready to get an output sample from the mixer.
    DWORD status = 0;
    MFT_OUTPUT_DATA_BUFFER dataBuffer = {};
    dataBuffer.pSample = sample.Get();
    HRESULT hr = m_mixer->ProcessOutput(0, 1, &dataBuffer, &status);
    // Important: Release any events returned from the ProcessOutput method.
    qt_evr_safe_release(&dataBuffer.pEvents);

    if (FAILED(hr)) {
        // Return the sample to the pool.
        m_samplePool.returnSample(sample);

        // Handle some known error codes from ProcessOutput.
        if (hr == MF_E_TRANSFORM_TYPE_NOT_SET) {
            // The mixer's format is not set. Negotiate a new format.
            hr = renegotiateMediaType();
        } else if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
            // There was a dynamic media type change. Clear our media type.
            setMediaType(NULL);
        } else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
            // The mixer needs more input.
            // We have to wait for the mixer to get more input.
            m_sampleNotify = false;
        }

        return hr;
    }

    // We got an output sample from the mixer.
    if (m_clock) {
        // Latency: Record the ending time for the ProcessOutput operation,
        // and notify the EVR of the latency.

        m_clock->GetCorrelatedTime(0, &mixerEndTime, &systemTime);

        LONGLONG latencyTime = mixerEndTime - mixerStartTime;
        notifyEvent(EC_PROCESSING_LATENCY, reinterpret_cast<LONG_PTR>(&latencyTime), 0);
    }

    // Set up notification for when the sample is released.
    hr = trackSample(sample);
    if (FAILED(hr))
        return hr;

    // Schedule the sample.
    if (m_frameStep.state == FrameStepNone)
        hr = deliverSample(sample);
    else // We are frame-stepping
        hr = deliverFrameStepSample(sample);

    if (FAILED(hr))
        return hr;

    m_prerolled = true; // We have presented at least one sample now.
    return S_OK;
}

HRESULT EVRCustomPresenter::deliverSample(const ComPtr<IMFSample> &sample)
{
    // If we are not actively playing, OR we are scrubbing (rate = 0),
    // then we need to present the sample immediately. Otherwise,
    // schedule it normally.

    bool presentNow = ((m_renderState != RenderStarted) ||  isScrubbing());

    HRESULT hr = m_scheduler.scheduleSample(sample, presentNow);

    if (FAILED(hr)) {
        // Notify the EVR that we have failed during streaming. The EVR will notify the
        // pipeline.

        notifyEvent(EC_ERRORABORT, hr, 0);
    }

    return hr;
}

HRESULT EVRCustomPresenter::deliverFrameStepSample(const ComPtr<IMFSample> &sample)
{
    HRESULT hr = S_OK;
    IUnknown *unk = NULL;

    // For rate 0, discard any sample that ends earlier than the clock time.
    if (isScrubbing() && m_clock && qt_evr_isSampleTimePassed(m_clock.Get(), sample.Get())) {
        // Discard this sample.
    } else if (m_frameStep.state >= FrameStepScheduled) {
        // A frame was already submitted. Put this sample on the frame-step queue,
        // in case we are asked to step to the next frame. If frame-stepping is
        // cancelled, this sample will be processed normally.
        m_frameStep.samples.append(sample);
    } else {
        // We're ready to frame-step.

        // Decrement the number of steps.
        if (m_frameStep.steps > 0)
            m_frameStep.steps--;

        if (m_frameStep.steps > 0) {
            // This is not the last step. Discard this sample.
        } else if (m_frameStep.state == FrameStepWaitingStart) {
            // This is the right frame, but the clock hasn't started yet. Put the
            // sample on the frame-step queue. When the clock starts, the sample
            // will be processed.
            m_frameStep.samples.append(sample);
        } else {
            // This is the right frame *and* the clock has started. Deliver this sample.
            hr = deliverSample(sample);
            if (FAILED(hr))
                goto done;

            // Query for IUnknown so that we can identify the sample later.
            // Per COM rules, an object always returns the same pointer when QI'ed for IUnknown.
            hr = sample->QueryInterface(IID_PPV_ARGS(&unk));
            if (FAILED(hr))
                goto done;

            m_frameStep.sampleNoRef = reinterpret_cast<DWORD_PTR>(unk); // No add-ref.

            // NOTE: We do not AddRef the IUnknown pointer, because that would prevent the
            // sample from invoking the OnSampleFree callback after the sample is presented.
            // We use this IUnknown pointer purely to identify the sample later; we never
            // attempt to dereference the pointer.

            m_frameStep.state = FrameStepScheduled;
        }
    }
done:
    qt_evr_safe_release(&unk);
    return hr;
}

HRESULT EVRCustomPresenter::trackSample(const ComPtr<IMFSample> &sample)
{
    IMFTrackedSample *tracked = NULL;

    HRESULT hr = sample->QueryInterface(IID_PPV_ARGS(&tracked));

    if (SUCCEEDED(hr))
        hr = tracked->SetAllocator(&m_sampleFreeCB, NULL);

    qt_evr_safe_release(&tracked);
    return hr;
}

void EVRCustomPresenter::releaseResources()
{
    // Increment the token counter to indicate that all existing video samples
    // are "stale." As these samples get released, we'll dispose of them.
    //
    // Note: The token counter is required because the samples are shared
    // between more than one thread, and they are returned to the presenter
    // through an asynchronous callback (onSampleFree). Without the token, we
    // might accidentally re-use a stale sample after the ReleaseResources
    // method returns.

    m_tokenCounter++;

    flush();

    m_samplePool.clear();

    m_presentEngine->releaseResources();
}

HRESULT EVRCustomPresenter::onSampleFree(IMFAsyncResult *result)
{
    IUnknown *object = NULL;
    IMFSample *sample = NULL;
    IUnknown *unk = NULL;
    UINT32 token;

    // Get the sample from the async result object.
    HRESULT hr = result->GetObject(&object);
    if (FAILED(hr))
        goto done;

    hr = object->QueryInterface(IID_PPV_ARGS(&sample));
    if (FAILED(hr))
        goto done;

    // If this sample was submitted for a frame-step, the frame step operation
    // is complete.

    if (m_frameStep.state == FrameStepScheduled) {
        // Query the sample for IUnknown and compare it to our cached value.
        hr = sample->QueryInterface(IID_PPV_ARGS(&unk));
        if (FAILED(hr))
            goto done;

        if (m_frameStep.sampleNoRef == reinterpret_cast<DWORD_PTR>(unk)) {
            // Notify the EVR.
            hr = completeFrameStep(sample);
            if (FAILED(hr))
                goto done;
        }

        // Note: Although object is also an IUnknown pointer, it is not
        // guaranteed to be the exact pointer value returned through
        // QueryInterface. Therefore, the second QueryInterface call is
        // required.
    }

    m_mutex.lock();

    token = MFGetAttributeUINT32(sample, MFSamplePresenter_SampleCounter, (UINT32)-1);

    if (token == m_tokenCounter) {
        // Return the sample to the sample pool.
        m_samplePool.returnSample(sample);
        // A free sample is available. Process more data if possible.
        processOutputLoop();
    }

    m_mutex.unlock();

done:
    if (FAILED(hr))
        notifyEvent(EC_ERRORABORT, hr, 0);
    qt_evr_safe_release(&object);
    qt_evr_safe_release(&sample);
    qt_evr_safe_release(&unk);
    return hr;
}

float EVRCustomPresenter::getMaxRate(bool thin)
{
    // Non-thinned:
    // If we have a valid frame rate and a monitor refresh rate, the maximum
    // playback rate is equal to the refresh rate. Otherwise, the maximum rate
    // is unbounded (FLT_MAX).

    // Thinned: The maximum rate is unbounded.

    float maxRate = FLT_MAX;
    MFRatio fps = { 0, 0 };
    UINT monitorRateHz = 0;

    if (!thin && m_mediaType) {
        qt_evr_getFrameRate(m_mediaType.Get(), &fps);
        monitorRateHz = m_presentEngine->refreshRate();

        if (fps.Denominator && fps.Numerator && monitorRateHz) {
            // Max Rate = Refresh Rate / Frame Rate
            maxRate = (float)MulDiv(monitorRateHz, fps.Denominator, fps.Numerator);
        }
    }

    return maxRate;
}

bool EVRCustomPresenter::event(QEvent *e)
{
    switch (int(e->type())) {
    case StartSurface:
        startSurface();
        return true;
    case StopSurface:
        stopSurface();
        return true;
    case PresentSample:
        presentSample(static_cast<PresentSampleEvent *>(e)->sample());
        return true;
    default:
        break;
    }
    return QObject::event(e);
}

void EVRCustomPresenter::startSurface()
{
    if (thread() != QThread::currentThread()) {
        QCoreApplication::postEvent(this, new QEvent(QEvent::Type(StartSurface)));
        return;
    }
}

void EVRCustomPresenter::stopSurface()
{
    if (thread() != QThread::currentThread()) {
        QCoreApplication::postEvent(this, new QEvent(QEvent::Type(StopSurface)));
        return;
    }
}

void EVRCustomPresenter::presentSample(const ComPtr<IMFSample> &sample)
{
    if (thread() != QThread::currentThread()) {
        QCoreApplication::postEvent(this, new PresentSampleEvent(sample));
        return;
    }

    if (!m_videoSink || !m_presentEngine->videoSurfaceFormat().isValid())
        return;

    QtVideo::Rotation rotation = [&] {
        ComPtr<IMFMediaType> inputStreamType;
        if (SUCCEEDED(m_mixer->GetInputCurrentType(0, inputStreamType.GetAddressOf()))) {
            auto rotation = static_cast<MFVideoRotationFormat>(
                    MFGetAttributeUINT32(inputStreamType.Get(), MF_MT_VIDEO_ROTATION, 0));
            switch (rotation) {
            case MFVideoRotationFormat_90:
                return QtVideo::Rotation::Clockwise90;
            case MFVideoRotationFormat_180:
                return QtVideo::Rotation::Clockwise180;
            case MFVideoRotationFormat_270:
                return QtVideo::Rotation::Clockwise270;
            case MFVideoRotationFormat_0:
            default:
                return QtVideo::Rotation::None;
            }
        }
        return QtVideo::Rotation::None;
    }();

    QVideoFrame frame = m_presentEngine->makeVideoFrame(sample, rotation);

    // Since start/end times are related to a position when the clock is started,
    // to have times from the beginning, need to adjust it by adding seeked position.
    if (m_positionOffset) {
        if (frame.startTime())
            frame.setStartTime(frame.startTime() + m_positionOffset);
        if (frame.endTime())
            frame.setEndTime(frame.endTime() + m_positionOffset);
    }

    m_videoSink->platformVideoSink()->setVideoFrame(frame);
}

void EVRCustomPresenter::positionChanged(qint64 position)
{
    m_positionOffset = position * 1000;
}

HRESULT setMixerSourceRect(IMFTransform *mixer, const MFVideoNormalizedRect &sourceRect)
{
    if (!mixer)
        return E_POINTER;

    IMFAttributes *attributes = NULL;

    HRESULT hr = mixer->GetAttributes(&attributes);
    if (SUCCEEDED(hr)) {
        hr = attributes->SetBlob(VIDEO_ZOOM_RECT, reinterpret_cast<const UINT8*>(&sourceRect),
                                 sizeof(sourceRect));
        attributes->Release();
    }
    return hr;
}

static QVideoFrameFormat::PixelFormat pixelFormatFromMediaType(IMFMediaType *type)
{
    GUID majorType;
    if (FAILED(type->GetMajorType(&majorType)))
        return QVideoFrameFormat::Format_Invalid;
    if (majorType != MFMediaType_Video)
        return QVideoFrameFormat::Format_Invalid;

    GUID subtype;
    if (FAILED(type->GetGUID(MF_MT_SUBTYPE, &subtype)))
        return QVideoFrameFormat::Format_Invalid;

    return QWindowsMultimediaUtils::pixelFormatFromMediaSubtype(subtype);
}

QT_END_NAMESPACE
