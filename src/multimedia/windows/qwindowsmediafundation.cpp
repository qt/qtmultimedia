// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsmediafundation_p.h"
#include "qwindowdefs_win.h"
#include <QDebug>
#include <QMutex>

QT_BEGIN_NAMESPACE

namespace {
struct Holder {
    ~Holder()
    {
        QMutexLocker locker(&mutex);
        instance = nullptr;
    }
    bool loadFailed = false;
    QBasicMutex mutex;
    std::unique_ptr<QWindowsMediaFundation> instance;
} holder;

}

QWindowsMediaFundation::~QWindowsMediaFundation() = default;

template<typename T> bool setProcAddress(QLibrary &lib, T &f, std::string_view name)
{
    f = reinterpret_cast<T>(lib.resolve(name.data()));
    return bool(f);
}

QWindowsMediaFundation *QWindowsMediaFundation::instance()
{
    QMutexLocker locker(&holder.mutex);
    if (holder.instance)
        return holder.instance.get();

    if (holder.loadFailed)
        return nullptr;

    std::unique_ptr<QWindowsMediaFundation> wmf(new QWindowsMediaFundation);
    if (wmf->m_mfplat.load()) {
        if (setProcAddress(wmf->m_mfplat, wmf->mfCreateMediaType, "MFCreateMediaType")
            && setProcAddress(wmf->m_mfplat, wmf->mfCreateMemoryBuffer, "MFCreateMemoryBuffer")
            && setProcAddress(wmf->m_mfplat, wmf->mfCreateSample, "MFCreateSample"))
        {
            holder.instance = std::move(wmf);
            return holder.instance.get();
        }
    }

    holder.loadFailed = true;
    return nullptr;
}

QT_END_NAMESPACE
