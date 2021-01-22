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

#include "directshowbasefilter.h"

#include "directshowpinenum.h"

#include <mutex>

QT_BEGIN_NAMESPACE

DirectShowBaseFilter::DirectShowBaseFilter()
    = default;

DirectShowBaseFilter::~DirectShowBaseFilter()
{
    if (m_clock) {
        m_clock->Release();
        m_clock = nullptr;
    }
}

HRESULT DirectShowBaseFilter::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_NULL;
    return S_OK;
}

HRESULT DirectShowBaseFilter::NotifyEvent(long eventCode, LONG_PTR eventParam1, LONG_PTR eventParam2)
{
    IMediaEventSink *sink = m_sink;
    if (sink) {
        if (eventCode == EC_COMPLETE)
            eventParam2 = (LONG_PTR)(IBaseFilter*)this;

        return sink->Notify(eventCode, eventParam1, eventParam2);
    }
    return E_NOTIMPL;
}

HRESULT DirectShowBaseFilter::Run(REFERENCE_TIME tStart)
{
    Q_UNUSED(tStart)
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    m_startTime = tStart;

    if (m_state == State_Stopped){
        HRESULT hr = Pause();
        if (FAILED(hr))
            return hr;
    }

    m_state = State_Running;

    return S_OK;
}

HRESULT DirectShowBaseFilter::Pause()
{
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    if (m_state == State_Stopped) {
        const QList<DirectShowPin *> pinList = pins();
        for (DirectShowPin *pin : pinList) {
            if (pin->isConnected()) {
                HRESULT hr = pin->setActive(true);
                if (FAILED(hr))
                    return hr;
            }
        }
    }

    m_state = State_Paused;

    return S_OK;
}

HRESULT DirectShowBaseFilter::Stop()
{
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    HRESULT hr = S_OK;

    if (m_state != State_Stopped) {
        const QList<DirectShowPin *> pinList = pins();
        for (DirectShowPin *pin : pinList) {
            if (pin->isConnected()) {
                HRESULT hrTmp = pin->setActive(false);
                if (FAILED(hrTmp) && SUCCEEDED(hr))
                    hr = hrTmp;
            }
        }
    }

    m_state = State_Stopped;

    return hr;
}

HRESULT DirectShowBaseFilter::GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *pState)
{
    Q_UNUSED(dwMilliSecsTimeout);

    if (!pState) {
        return E_POINTER;
    } else {
        const std::lock_guard<QRecursiveMutex> locker(m_mutex);

        *pState = m_state;

        return S_OK;
    }
}

HRESULT DirectShowBaseFilter::SetSyncSource(IReferenceClock *pClock)
{
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    if (m_clock)
        m_clock->Release();

    m_clock = pClock;

    if (m_clock)
        m_clock->AddRef();

    return S_OK;
}

HRESULT DirectShowBaseFilter::GetSyncSource(IReferenceClock **ppClock)
{
    if (!ppClock)
        return E_POINTER;

    if (!m_clock) {
        *ppClock = nullptr;
        return S_FALSE;
    }
    m_clock->AddRef();
    *ppClock = m_clock;
    return S_OK;
}

HRESULT DirectShowBaseFilter::EnumPins(IEnumPins **ppEnum)
{
    if (!ppEnum)
        return E_POINTER;
    *ppEnum = new DirectShowPinEnum(this);
    return S_OK;
}

HRESULT DirectShowBaseFilter::FindPin(LPCWSTR Id, IPin **ppPin)
{
    if (!ppPin || !Id)
        return E_POINTER;

    const std::lock_guard<QRecursiveMutex> locker(m_mutex);
    const QList<DirectShowPin *> pinList = pins();
    for (DirectShowPin *pin : pinList) {
        if (pin->name() == QStringView(Id)) {
            pin->AddRef();
            *ppPin = pin;
            return S_OK;
        }
    }

    *ppPin = nullptr;
    return VFW_E_NOT_FOUND;
}

HRESULT DirectShowBaseFilter::JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName)
{
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    m_filterName = QString::fromWCharArray(pName);
    m_graph = pGraph;
    m_sink = nullptr;

    if (m_graph) {
        if (SUCCEEDED(m_graph->QueryInterface(IID_PPV_ARGS(&m_sink))))
            m_sink->Release(); // we don't keep a reference on it
    }

    return S_OK;
}

HRESULT DirectShowBaseFilter::QueryFilterInfo(FILTER_INFO *pInfo)
{
    if (!pInfo)
        return E_POINTER;

    QString name = m_filterName;

    if (name.length() >= MAX_FILTER_NAME)
        name.truncate(MAX_FILTER_NAME - 1);

    int length = name.toWCharArray(pInfo->achName);
    pInfo->achName[length] = '\0';

    if (m_graph)
        m_graph->AddRef();

    pInfo->pGraph = m_graph;

    return S_OK;
}

HRESULT DirectShowBaseFilter::QueryVendorInfo(LPWSTR *pVendorInfo)
{
    Q_UNUSED(pVendorInfo);
    return E_NOTIMPL;
}

QT_END_NAMESPACE
