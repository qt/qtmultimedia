// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSASFPICTURE_P_H
#define QWINDOWSASFPICTURE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qimage.h>

#include <wtypes.h>

#include <mfapi.h>
#include <mfidl.h>

QT_BEGIN_NAMESPACE

// MinGW's mfidl.h declares ASF_FLAT_PICTURE without the #pragma pack(1) that the
// Windows SDK applies, making sizeof() 8 there instead of 5. Use our own packed
// definition on both toolchains, so there is a single code path guarded by the
// static_assert below.
#pragma pack(push, 1)
struct QMM_ASF_FLAT_PICTURE
{
    BYTE bPictureType;
    DWORD dwDataLen;
};
#pragma pack(pop)
static_assert(sizeof(QMM_ASF_FLAT_PICTURE) == 5);
static_assert(alignof(QMM_ASF_FLAT_PICTURE) == 1);

QImage imageFromAsfFlatPicture(const BLOB &blob);

QT_END_NAMESPACE

#endif
