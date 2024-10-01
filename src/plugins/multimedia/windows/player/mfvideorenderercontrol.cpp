// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "mfvideorenderercontrol_p.h"
#include "mfactivate_p.h"

#include "evrcustompresenter_p.h"

#include <private/qplatformvideosink_p.h>

QT_BEGIN_NAMESPACE

class EVRCustomPresenterActivate : public MFAbstractActivate
{
public:
    EVRCustomPresenterActivate(QVideoSink *sink);

    STDMETHODIMP ActivateObject(REFIID riid, void **ppv) override;
    STDMETHODIMP ShutdownObject() override;
    STDMETHODIMP DetachObject() override;

    void setSink(QVideoSink *sink);
    void setCropRect(QRect cropRect);

private:
    // Destructor is not public. Caller should call Release.
    ~EVRCustomPresenterActivate() override { }

    ComPtr<EVRCustomPresenter> m_presenter;
    QVideoSink *m_videoSink;
    QRect m_cropRect;
    QMutex m_mutex;
};


MFVideoRendererControl::MFVideoRendererControl(QObject *parent)
    : QObject(parent)
{
}

MFVideoRendererControl::~MFVideoRendererControl()
{
    releaseActivate();
}

void MFVideoRendererControl::releaseActivate()
{
    if (m_sink)
        m_sink->platformVideoSink()->setVideoFrame(QVideoFrame());

    if (m_presenterActivate) {
        m_presenterActivate->ShutdownObject();
        m_presenterActivate.Reset();
    }

    if (m_currentActivate) {
        m_currentActivate->ShutdownObject();
        m_currentActivate.Reset();
    }
}

void MFVideoRendererControl::setSink(QVideoSink *sink)
{
    m_sink = sink;

    if (m_presenterActivate)
        m_presenterActivate->setSink(m_sink);
}

void MFVideoRendererControl::setCropRect(const QRect &cropRect)
{
    if (m_presenterActivate)
        m_presenterActivate->setCropRect(cropRect);
}

IMFActivate* MFVideoRendererControl::createActivate()
{
    releaseActivate();

    if (m_sink) {
        // Create the EVR media sink, but replace the presenter with our own
        if (SUCCEEDED(MFCreateVideoRendererActivate(::GetShellWindow(), &m_currentActivate))) {
            m_presenterActivate = makeComObject<EVRCustomPresenterActivate>(m_sink);
            m_currentActivate->SetUnknown(MF_ACTIVATE_CUSTOM_VIDEO_PRESENTER_ACTIVATE,
                                          m_presenterActivate.Get());
        }
    }

    return m_currentActivate.Get();
}

EVRCustomPresenterActivate::EVRCustomPresenterActivate(QVideoSink *sink)
    : MFAbstractActivate(), m_videoSink(sink)
{ }

HRESULT EVRCustomPresenterActivate::ActivateObject(REFIID riid, void **ppv)
{
    if (!ppv)
        return E_INVALIDARG;
    QMutexLocker locker(&m_mutex);
    if (!m_presenter) {
        m_presenter = makeComObject<EVRCustomPresenter>(m_videoSink);
        m_presenter->setCropRect(m_cropRect);
    }
    return m_presenter->QueryInterface(riid, ppv);
}

HRESULT EVRCustomPresenterActivate::ShutdownObject()
{
    // The presenter does not implement IMFShutdown so
    // this function is the same as DetachObject()
    return DetachObject();
}

HRESULT EVRCustomPresenterActivate::DetachObject()
{
    QMutexLocker locker(&m_mutex);
    if (m_presenter) {
        m_presenter.Reset();
    }
    return S_OK;
}

void EVRCustomPresenterActivate::setSink(QVideoSink *sink)
{
    QMutexLocker locker(&m_mutex);
    if (m_videoSink == sink)
        return;

    m_videoSink = sink;

    if (m_presenter)
        m_presenter->setSink(sink);
}

void EVRCustomPresenterActivate::setCropRect(QRect cropRect)
{
    QMutexLocker locker(&m_mutex);
    if (m_cropRect == cropRect)
        return;

    m_cropRect = cropRect;

    if (m_presenter)
        m_presenter->setCropRect(cropRect);
}

QT_END_NAMESPACE
