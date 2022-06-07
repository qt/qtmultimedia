// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>

#if !defined(PA_API_VERSION) || PA_API_VERSION-0 != 12
# error "Incompatible PulseAudio API version"
#endif

int main(int, char **)
{
    const char *headers = pa_get_headers_version();
    const char *library = pa_get_library_version();
    pa_glib_mainloop_new(0);
    return (headers - library) * 0;
}
