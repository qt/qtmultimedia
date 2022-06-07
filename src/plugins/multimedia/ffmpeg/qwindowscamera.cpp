// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowscamera_p.h"
#include "qsemaphore.h"
#include "qmutex.h"

#include <private/qmemoryvideobuffer_p.h>
#include <private/qwindowsmfdefs_p.h>

#include <mfapi.h>
#include <mfidl.h>
#include <Mferror.h>
#include <Mfreadwrite.h>

#include <system_error>

QT_BEGIN_NAMESPACE

class CameraReaderCallback : public IMFSourceReaderCallback
{
public:
    CameraReaderCallback() : m_cRef(1) {}
    virtual ~CameraReaderCallback() {}

    //from IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObject) override
    {
        if (!ppvObject)
            return E_POINTER;
        if (riid == IID_IMFSourceReaderCallback) {
            *ppvObject = static_cast<IMFSourceReaderCallback*>(this);
        } else if (riid == IID_IUnknown) {
            *ppvObject = static_cast<IUnknown*>(static_cast<IMFSourceReaderCallback*>(this));
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
    LONG m_cRef;
    ActiveCamera *m_activeCamera = nullptr;
    QMutex m_mutex;
};

static QWindowsIUPointer<IMFSourceReader> createCameraReader(IMFMediaSource *mediaSource,
                                                             const QWindowsIUPointer<CameraReaderCallback> &callback)
{
    QWindowsIUPointer<IMFSourceReader> sourceReader;
    QWindowsIUPointer<IMFAttributes> readerAttributes;

    HRESULT hr = MFCreateAttributes(readerAttributes.address(), 1);
    if (SUCCEEDED(hr)) {
        hr = readerAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, callback.get());
        if (SUCCEEDED(hr)) {
            hr = MFCreateSourceReaderFromMediaSource(mediaSource, readerAttributes.get(), sourceReader.address());
            if (SUCCEEDED(hr))
                return sourceReader;
        }
    }

    qWarning() << "Failed to create camera IMFSourceReader" << hr;
    return sourceReader;
}

static QWindowsIUPointer<IMFMediaSource> createCameraSource(const QString &deviceId)
{
    QWindowsIUPointer<IMFMediaSource> mediaSource;
    QWindowsIUPointer<IMFAttributes> sourceAttributes;
    HRESULT hr = MFCreateAttributes(sourceAttributes.address(), 2);
    if (SUCCEEDED(hr)) {
        hr = sourceAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, QMM_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
        if (SUCCEEDED(hr)) {
            hr = sourceAttributes->SetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                                             reinterpret_cast<LPCWSTR>(deviceId.utf16()));
            if (SUCCEEDED(hr)) {
                hr = MFCreateDeviceSource(sourceAttributes.get(), mediaSource.address());
                if (SUCCEEDED(hr))
                    return mediaSource;
            }
        }
    }
    qWarning() << "Failed to create camera IMFMediaSource" << hr;
    return mediaSource;
}

static int calculateVideoFrameStride(IMFSourceReader *sourceReader, qsizetype formatIndex, int width)
{
    Q_ASSERT(sourceReader);
    QWindowsIUPointer<IMFMediaType> videoType;
    HRESULT hr = sourceReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                                  formatIndex, videoType.address());
    if (SUCCEEDED(hr)) {
        GUID subtype = GUID_NULL;
        hr = videoType->GetGUID(MF_MT_SUBTYPE, &subtype);
        if (SUCCEEDED(hr)) {
            LONG stride = 0;
            hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, width, &stride);
            if (SUCCEEDED(hr))
                return int(stride);
        }
    }

    qWarning() << "Failed to calculate video stride" << hr;
    return 0;
}

static bool setCameraReaderFormat(IMFSourceReader *sourceReader, qsizetype formatIndex)
{
    Q_ASSERT(sourceReader);

    QWindowsIUPointer<IMFMediaType> videoType;
    HRESULT hr = sourceReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                                  formatIndex, videoType.address());
    if (SUCCEEDED(hr)) {
        hr = sourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                               nullptr, videoType.get());
        if (SUCCEEDED(hr)) {
            return true;
        } else {
            qWarning() << "Failed to set video format" << hr << std::system_category().message(hr).c_str();
        }
    } else {
        qWarning() << "Failed to select video format at index" << formatIndex << hr << std::system_category().message(hr).c_str();
    }

    return false;
}

class ActiveCamera {
public:
    ActiveCamera() = delete;

    static std::unique_ptr<ActiveCamera> create(QWindowsCamera &wc, const QCameraDevice &device, const QCameraFormat &format)
    {
        auto ac = std::unique_ptr<ActiveCamera>(new ActiveCamera(wc));
        ac->m_source = createCameraSource(device.id());
        if (!ac->m_source)
            return {};

        ac->m_readerCallback = QWindowsIUPointer<CameraReaderCallback>(new CameraReaderCallback);
        ac->m_readerCallback->setActiveCamera(ac.get());
        ac->m_reader = createCameraReader(ac->m_source.get(), ac->m_readerCallback);
        if (!ac->m_reader)
            return {};

        qsizetype index = device.videoFormats().indexOf(format);
        if (index < 0)
            return {};

        if (!ac->setFormat(format, index))
            return {};

        return ac;
    }

    bool setFormat(const QCameraFormat &format, int formatIndex)
    {
        m_reader->Flush(MF_SOURCE_READER_FIRST_VIDEO_STREAM);
        m_flushWait.acquire();

        if (!setCameraReaderFormat(m_reader.get(), formatIndex))
            return false;

        m_frameFormat = { format.resolution(), format.pixelFormat() };
        m_videoFrameStride = calculateVideoFrameStride(m_reader.get(), formatIndex, format.resolution().width());

        m_reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr,
                             nullptr, nullptr, nullptr);
        return true;
    }

    void onReadSample(HRESULT status, LONGLONG timestamp, IMFSample *sample)
    {
        if (FAILED(status)) {
            emit m_windowsCamera.error(int(status), std::system_category().message(status).c_str());
            return;
        }

        if (sample) {
            QWindowsIUPointer<IMFMediaBuffer> mediaBuffer;
            if (SUCCEEDED(sample->ConvertToContiguousBuffer(mediaBuffer.address()))) {

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
        m_reader->Flush(MF_SOURCE_READER_FIRST_VIDEO_STREAM);
        m_flushWait.acquire();
        m_readerCallback->setActiveCamera(nullptr);
    }

private:
    explicit ActiveCamera(QWindowsCamera &wc) : m_windowsCamera(wc), m_flushWait(0) {};

    QWindowsCamera &m_windowsCamera;

    QSemaphore m_flushWait;

    QWindowsIUPointer<IMFMediaSource> m_source;
    QWindowsIUPointer<IMFSourceReader> m_reader;
    QWindowsIUPointer<CameraReaderCallback> m_readerCallback;

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
        emit activeChanged(false);
        m_active.reset();
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
    qsizetype index = m_cameraDevice.videoFormats().indexOf(format);
    if (index < 0)
        return false;

    bool ok = m_active ? m_active->setFormat(format, index) : true;
    if (ok)
        m_cameraFormat = format;

    return ok;
}

QT_END_NAMESPACE
