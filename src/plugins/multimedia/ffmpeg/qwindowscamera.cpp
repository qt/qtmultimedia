// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowscamera_p.h"
#include "qsemaphore.h"
#include "qmutex.h"

#include <private/qmemoryvideobuffer_p.h>
#include <private/qwindowsmfdefs_p.h>
#include <private/qwindowsmultimediautils_p.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mfreadwrite.h>

#include <system_error>

QT_BEGIN_NAMESPACE

using namespace QWindowsMultimediaUtils;

class CameraReaderCallback : public IMFSourceReaderCallback
{
public:
    CameraReaderCallback() : m_cRef(1) { }

    //from IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObject) override
    {
        if (!ppvObject)
            return E_POINTER;
        if (riid == IID_IMFSourceReaderCallback) {
            *ppvObject = static_cast<IMFSourceReaderCallback*>(this);
        } else if (riid == IID_IUnknown) {
            *ppvObject = static_cast<IUnknown *>(this);
        } else {
            *ppvObject =  nullptr;
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    STDMETHODIMP_(ULONG) AddRef() override
    {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG) Release() override
    {
        LONG cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0) {
            delete this;
        }
        return cRef;
    }

    //from IMFSourceReaderCallback
    STDMETHODIMP OnReadSample(HRESULT status, DWORD, DWORD, LONGLONG timestamp, IMFSample *sample) override;
    STDMETHODIMP OnFlush(DWORD) override;
    STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *) override { return S_OK; }

    void setActiveCamera(ActiveCamera *activeCamera)
    {
        QMutexLocker locker(&m_mutex);
        m_activeCamera = activeCamera;
    }
private:
    // Destructor is private. Caller should call Release.
    virtual ~CameraReaderCallback() { }

    LONG m_cRef;
    ActiveCamera *m_activeCamera = nullptr;
    QMutex m_mutex;
};

static ComPtr<IMFSourceReader> createCameraReader(IMFMediaSource *mediaSource,
                                                             const ComPtr<CameraReaderCallback> &callback)
{
    ComPtr<IMFSourceReader> sourceReader;
    ComPtr<IMFAttributes> readerAttributes;

    HRESULT hr = MFCreateAttributes(readerAttributes.GetAddressOf(), 1);
    if (SUCCEEDED(hr)) {
        hr = readerAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, callback.Get());
        if (SUCCEEDED(hr)) {
            hr = MFCreateSourceReaderFromMediaSource(mediaSource, readerAttributes.Get(), sourceReader.GetAddressOf());
            if (SUCCEEDED(hr))
                return sourceReader;
        }
    }

    qWarning() << "Failed to create camera IMFSourceReader" << hr;
    return sourceReader;
}

static ComPtr<IMFMediaSource> createCameraSource(const QString &deviceId)
{
    ComPtr<IMFMediaSource> mediaSource;
    ComPtr<IMFAttributes> sourceAttributes;
    HRESULT hr = MFCreateAttributes(sourceAttributes.GetAddressOf(), 2);
    if (SUCCEEDED(hr)) {
        hr = sourceAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, QMM_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
        if (SUCCEEDED(hr)) {
            hr = sourceAttributes->SetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                                             reinterpret_cast<LPCWSTR>(deviceId.utf16()));
            if (SUCCEEDED(hr)) {
                hr = MFCreateDeviceSource(sourceAttributes.Get(), mediaSource.GetAddressOf());
                if (SUCCEEDED(hr))
                    return mediaSource;
            }
        }
    }
    qWarning() << "Failed to create camera IMFMediaSource" << hr;
    return mediaSource;
}

static int calculateVideoFrameStride(IMFMediaType *videoType, int width)
{
    Q_ASSERT(videoType);

    GUID subtype = GUID_NULL;
    HRESULT hr = videoType->GetGUID(MF_MT_SUBTYPE, &subtype);
    if (SUCCEEDED(hr)) {
        LONG stride = 0;
        hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, width, &stride);
        if (SUCCEEDED(hr))
            return int(qAbs(stride));
    }

    qWarning() << "Failed to calculate video stride" << errorString(hr);
    return 0;
}

static bool setCameraReaderFormat(IMFSourceReader *sourceReader, IMFMediaType *videoType)
{
    Q_ASSERT(sourceReader);
    Q_ASSERT(videoType);

    HRESULT hr = sourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr,
                                                   videoType);
    if (FAILED(hr))
        qWarning() << "Failed to set video format" << errorString(hr);

    return SUCCEEDED(hr);
}

static ComPtr<IMFMediaType> findVideoType(IMFSourceReader *reader,
                                                     const QCameraFormat &format)
{
    for (DWORD i = 0;; ++i) {
        ComPtr<IMFMediaType> candidate;
        HRESULT hr = reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, i,
                                                candidate.GetAddressOf());
        if (FAILED(hr))
            break;

        GUID subtype = GUID_NULL;
        if (FAILED(candidate->GetGUID(MF_MT_SUBTYPE, &subtype)))
            continue;

        if (format.pixelFormat() != pixelFormatFromMediaSubtype(subtype))
            continue;

        UINT32 width = 0u;
        UINT32 height = 0u;
        if (FAILED(MFGetAttributeSize(candidate.Get(), MF_MT_FRAME_SIZE, &width, &height)))
            continue;

        if (format.resolution() != QSize{ int(width), int(height) })
            continue;

        return candidate;
    }
    return {};
}

class ActiveCamera {
public:
    static std::unique_ptr<ActiveCamera> create(QWindowsCamera &wc, const QCameraDevice &device, const QCameraFormat &format)
    {
        auto ac = std::unique_ptr<ActiveCamera>(new ActiveCamera(wc));
        ac->m_source = createCameraSource(device.id());
        if (!ac->m_source)
            return {};

        ac->m_readerCallback = makeComObject<CameraReaderCallback>();
        ac->m_readerCallback->setActiveCamera(ac.get());
        ac->m_reader = createCameraReader(ac->m_source.Get(), ac->m_readerCallback);
        if (!ac->m_reader)
            return {};

        if (!ac->setFormat(format))
            return {};

        return ac;
    }

    bool setFormat(const QCameraFormat &format)
    {
        flush();

        auto videoType = findVideoType(m_reader.Get(), format);
        if (videoType) {
            if (setCameraReaderFormat(m_reader.Get(), videoType.Get())) {
                m_frameFormat = { format.resolution(), format.pixelFormat() };
                m_videoFrameStride =
                        calculateVideoFrameStride(videoType.Get(), format.resolution().width());
            }
        }

        m_reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, nullptr, nullptr,
                             nullptr);
        return true;
    }

    void onReadSample(HRESULT status, LONGLONG timestamp, IMFSample *sample)
    {
        if (FAILED(status)) {
            emit m_windowsCamera.error(int(status), std::system_category().message(status).c_str());
            return;
        }

        if (sample) {
            ComPtr<IMFMediaBuffer> mediaBuffer;
            if (SUCCEEDED(sample->ConvertToContiguousBuffer(mediaBuffer.GetAddressOf()))) {

                DWORD bufLen = 0;
                BYTE *buffer = nullptr;
                if (SUCCEEDED(mediaBuffer->Lock(&buffer, nullptr, &bufLen))) {
                    QByteArray bytes(reinterpret_cast<char*>(buffer), qsizetype(bufLen));
                    QVideoFrame frame(new QMemoryVideoBuffer(bytes, m_videoFrameStride), m_frameFormat);

                    // WMF uses 100-nanosecond units, Qt uses microseconds
                    frame.setStartTime(timestamp / 10);

                    LONGLONG duration = -1;
                    if (SUCCEEDED(sample->GetSampleDuration(&duration)))
                        frame.setEndTime((timestamp + duration) / 10);

                    emit m_windowsCamera.newVideoFrame(frame);
                    mediaBuffer->Unlock();
                }
            }
        }

        m_reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr,
                             nullptr, nullptr, nullptr);
    }

    void onFlush()
    {
        m_flushWait.release();
    }

    ~ActiveCamera()
    {
        flush();
        m_readerCallback->setActiveCamera(nullptr);
    }

private:
    explicit ActiveCamera(QWindowsCamera &wc) : m_windowsCamera(wc), m_flushWait(0) {};

    void flush()
    {
        if (SUCCEEDED(m_reader->Flush(MF_SOURCE_READER_FIRST_VIDEO_STREAM))) {
            m_flushWait.acquire();
        }
    }

    QWindowsCamera &m_windowsCamera;

    QSemaphore m_flushWait;

    ComPtr<IMFMediaSource> m_source;
    ComPtr<IMFSourceReader> m_reader;
    ComPtr<CameraReaderCallback> m_readerCallback;

    QVideoFrameFormat m_frameFormat;
    int m_videoFrameStride = 0;
};

STDMETHODIMP CameraReaderCallback::OnReadSample(HRESULT status, DWORD, DWORD, LONGLONG timestamp, IMFSample *sample)
{
    QMutexLocker locker(&m_mutex);
    if (m_activeCamera)
        m_activeCamera->onReadSample(status, timestamp, sample);

    return status;
}

STDMETHODIMP CameraReaderCallback::OnFlush(DWORD)
{
    QMutexLocker locker(&m_mutex);
    if (m_activeCamera)
        m_activeCamera->onFlush();
    return S_OK;
}

QWindowsCamera::QWindowsCamera(QCamera *camera)
    : QPlatformCamera(camera)
{
    m_cameraDevice = camera ? camera->cameraDevice() : QCameraDevice{};
}

QWindowsCamera::~QWindowsCamera()
{
    QWindowsCamera::setActive(false);
}

void QWindowsCamera::setActive(bool active)
{
    if (bool(m_active) == active)
        return;

    if (active) {
        if (m_cameraDevice.isNull())
            return;

        if (m_cameraFormat.isNull())
            m_cameraFormat = findBestCameraFormat(m_cameraDevice);

        m_active = ActiveCamera::create(*this, m_cameraDevice, m_cameraFormat);
        if (m_active)
            activeChanged(true);

    } else {
        m_active.reset();
        emit activeChanged(false);
    }
}

void QWindowsCamera::setCamera(const QCameraDevice &camera)
{
    bool active = bool(m_active);
    if (active)
        setActive(false);
    m_cameraDevice = camera;
    m_cameraFormat = {};
    if (active)
        setActive(true);
}

bool QWindowsCamera::setCameraFormat(const QCameraFormat &format)
{
    if (format.isNull())
        return false;

    bool ok = m_active ? m_active->setFormat(format) : true;
    if (ok)
        m_cameraFormat = format;

    return ok;
}

QT_END_NAMESPACE
