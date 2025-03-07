// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsmediafoundation_p.h"
#include <QtCore/qdebug.h>

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

    if (!m_mf.load(false))
        return;

    if (!m_mfreadwrite.load(false))
        return;

    m_valid = setProcAddress(m_mfplat, mfStartup, "MFStartup")
            && setProcAddress(m_mfplat, mfShutdown, "MFShutdown")
            && setProcAddress(m_mfplat, mfCreateMediaType, "MFCreateMediaType")
            && setProcAddress(m_mfplat, mfCreateMemoryBuffer, "MFCreateMemoryBuffer")
            && setProcAddress(m_mfplat, mfCreateSample, "MFCreateSample")
            && setProcAddress(m_mfplat, mfCreateAttributes, "MFCreateAttributes")
            && setProcAddress(m_mf, mfEnumDeviceSources, "MFEnumDeviceSources")
            && setProcAddress(m_mfreadwrite, mfCreateSourceReaderFromMediaSource,
                              "MFCreateSourceReaderFromMediaSource");

    Q_ASSERT(m_valid); // If it is not valid at this point, we have a programming bug
}

QWindowsMediaFoundation::~QWindowsMediaFoundation() = default;

bool QWindowsMediaFoundation::valid() const
{
    return m_valid;
}

QMFRuntimeInit::QMFRuntimeInit(QWindowsMediaFoundation *wmf)
    : m_wmf{ wmf }, m_initResult{ wmf ? m_wmf->mfStartup(MF_VERSION, MFSTARTUP_FULL) : E_FAIL }
{
    if (m_initResult != S_OK)
        qErrnoWarning(m_initResult, "Failed to initialize Windows Media Foundation");
}

QMFRuntimeInit::~QMFRuntimeInit()
{
    // According to documentation MFShutdown should be called
    // also when MFStartup failed. This is wrong.
    if (FAILED(m_initResult))
        return;

    const HRESULT hr = m_wmf->mfShutdown();
    if (hr != S_OK)
        qErrnoWarning(hr, "Failed to shut down Windows Media Foundation");
}

QT_END_NAMESPACE
