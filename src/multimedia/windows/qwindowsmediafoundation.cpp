// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsmediafoundation_p.h"

QT_BEGIN_NAMESPACE

namespace {

Q_GLOBAL_STATIC(QWindowsMediaFoundation, s_wmf);

template <typename T>
bool setProcAddress(QSystemLibrary &lib, T &f, const char name[])
{
    f = reinterpret_cast<T>(lib.resolve(name));
    return static_cast<bool>(f);
}

} // namespace

QWindowsMediaFoundation *QWindowsMediaFoundation::instance()
{
    if (s_wmf->valid())
        return s_wmf;

    return nullptr;
}

QWindowsMediaFoundation::QWindowsMediaFoundation()
{
    if (!m_mfplat.load(false))
        return;

    m_valid = setProcAddress(m_mfplat, mfCreateMediaType, "MFCreateMediaType")
            && setProcAddress(m_mfplat, mfCreateMemoryBuffer, "MFCreateMemoryBuffer")
            && setProcAddress(m_mfplat, mfCreateSample, "MFCreateSample");

    Q_ASSERT(m_valid); // If it is not valid at this point, we have a programming bug
}

QWindowsMediaFoundation::~QWindowsMediaFoundation() = default;

bool QWindowsMediaFoundation::valid() const
{
    return m_valid;
}

QT_END_NAMESPACE
