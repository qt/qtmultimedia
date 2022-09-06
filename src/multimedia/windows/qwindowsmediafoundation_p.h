// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSMEDIAFOUNDATION_H
#define QWINDOWSMEDIAFOUNDATION_H

#include <private/qtmultimediaglobal_p.h>
#include <QtCore/qt_windows.h>
#include <QLibrary>
#include <memory>
#include <mfapi.h>

struct IMFMediaType;

QT_BEGIN_NAMESPACE

class QWindowsMediaFoundation
{
public:
    ~QWindowsMediaFoundation();

    static QWindowsMediaFoundation *instance();

    decltype(&::MFCreateMediaType) mfCreateMediaType = nullptr;
    decltype(&::MFCreateMemoryBuffer) mfCreateMemoryBuffer = nullptr;
    decltype(&::MFCreateSample) mfCreateSample = nullptr;

private:
    QWindowsMediaFoundation() : m_mfplat(QStringLiteral("Mfplat.dll")) {}

    QLibrary m_mfplat;
};

QT_END_NAMESPACE

#endif // QWINDOWSMEDIAFOUNDATION_H
