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

#ifndef DIRECTSHOWUTILS_H
#define DIRECTSHOWUTILS_H

#include "directshowglobal.h"

QT_BEGIN_NAMESPACE

namespace DirectShowUtils
{
template <typename T>
void safeRelease(T **iface) {
    if (!iface)
        return;

    if (!*iface)
        return;

    (*iface)->Release();
    *iface = nullptr;
}

template <typename T>
struct ScopedSafeRelease
{
    T **iunknown;
    ~ScopedSafeRelease()
    {
        DirectShowUtils::safeRelease(iunknown);
    }
};

bool getPin(IBaseFilter *filter, PIN_DIRECTION pinDirection, REFGUID category, IPin **pin, HRESULT *hrOut);
bool isPinConnected(IPin *pin, HRESULT *hrOut = nullptr);
bool hasPinDirection(IPin *pin, PIN_DIRECTION direction, HRESULT *hrOut = nullptr);
bool matchPin(IPin *pin, PIN_DIRECTION pinDirection, BOOL shouldBeConnected, HRESULT *hrOut = nullptr);
bool findUnconnectedPin(IBaseFilter *filter, PIN_DIRECTION pinDirection, IPin **pin, HRESULT *hrOut = nullptr);
bool connectFilters(IGraphBuilder *graph, IPin *outputPin, IBaseFilter *filter, HRESULT *hrOut = nullptr);
bool connectFilters(IGraphBuilder *graph, IBaseFilter *filter, IPin *inputPin, HRESULT *hrOut = nullptr);
bool connectFilters(IGraphBuilder *graph,
                    IBaseFilter *upstreamFilter,
                    IBaseFilter *downstreamFilter,
                    bool autoConnect = false,
                    HRESULT *hrOut = nullptr);

void CoInitializeIfNeeded();
void CoUninitializeIfNeeded();

}

QT_END_NAMESPACE

#endif // DIRECTSHOWUTILS_H
