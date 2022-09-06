// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSMEDIAFUNDATION_H
#define QWINDOWSMEDIAFUNDATION_H

#include <private/qtmultimediaglobal_p.h>
#include <QtCore/qt_windows.h>
#include <QLibrary>
#include <memory>
#include <mfapi.h>

struct IMFMediaType;

QT_BEGIN_NAMESPACE

class QWindowsMediaFundation
{
public:
    ~QWindowsMediaFundation();

    static QWindowsMediaFundation *instance();

    decltype(&::MFCreateMediaType) mfCreateMediaType = nullptr;
    decltype(&::MFCreateMemoryBuffer) mfCreateMemoryBuffer = nullptr;
    decltype(&::MFCreateSample) mfCreateSample = nullptr;

private:
    QWindowsMediaFundation() : m_mfplat(QStringLiteral("Mfplat.dll")) {}

    QLibrary m_mfplat;
};

QT_END_NAMESPACE

#endif // QWINDOWSMEDIAFUNDATION_H
