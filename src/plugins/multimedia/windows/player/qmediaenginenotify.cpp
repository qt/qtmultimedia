// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediaenginenotify_p.h"

QT_BEGIN_NAMESPACE

STDMETHODIMP QMediaEngineNotify::EventNotify(DWORD event, DWORD_PTR param1, DWORD param2)
{
    emit eventNotified(event, param1, param2);
    return S_OK;
}

QT_END_NAMESPACE
