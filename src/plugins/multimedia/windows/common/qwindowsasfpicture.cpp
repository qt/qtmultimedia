// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qwindowsasfpicture_p.h"

QT_BEGIN_NAMESPACE

QImage imageFromAsfFlatPicture(const BLOB &blob)
{
    if (blob.cbSize <= sizeof(QMM_ASF_FLAT_PICTURE))
        return {};

    const auto *pic = reinterpret_cast<const QMM_ASF_FLAT_PICTURE *>(blob.pBlobData);
    const BYTE *p = blob.pBlobData + sizeof(QMM_ASF_FLAT_PICTURE);
    const BYTE *end = blob.pBlobData + blob.cbSize;

    // Skip MIME type (null-terminated UTF-16)
    while (p + 1 < end && (p[0] || p[1]))
        p += 2;
    p += 2;
    if (p > end)
        return {};

    // Skip description (null-terminated UTF-16)
    while (p + 1 < end && (p[0] || p[1]))
        p += 2;
    p += 2;
    if (p > end || pic->dwDataLen > static_cast<DWORD>(end - p))
        return {};

    QImage img;
    img.loadFromData(p, pic->dwDataLen);
    return img;
}

QT_END_NAMESPACE
