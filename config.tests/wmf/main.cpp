// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <d3d9.h>
#include <evr9.h>
#include <mmdeviceapi.h>

int main(int, char**)
{
    HRESULT hr = MENonFatalError;
    if (SUCCEEDED(hr)) {
        return 1;
    }
    return 0;
}
