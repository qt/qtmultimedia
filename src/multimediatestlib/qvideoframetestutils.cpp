// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qvideoframetestutils_p.h"

#include <cstring>

namespace QtMultimediaTest {

QByteArray makeBytes(std::initializer_list<uchar> bytes)
{
    return QByteArray(reinterpret_cast<const char *>(bytes.begin()), qsizetype(bytes.size()));
}

void fillPlaneRows(uchar *data, qsizetype stride, int width, int height, QByteArrayView texel)
{
    for (int row = 0; row < height; ++row) {
        uchar *rowData = data + row * stride;
        for (int col = 0; col < width; ++col)
            std::memcpy(rowData + col * texel.size(), texel.constData(), texel.size());
    }
}

q23::expected<void, QString> comparePlaneRows(const uchar *data, qsizetype stride, int width,
                                              int height, QByteArrayView texel)
{
    for (int row = 0; row < height; ++row) {
        const uchar *rowData = data + row * stride;
        for (int col = 0; col < width; ++col) {
            if (std::memcmp(rowData + col * texel.size(), texel.constData(), texel.size()) != 0)
                return q23::unexpected(
                        QStringLiteral("mismatch at row %1, column %2").arg(row).arg(col));
        }
    }
    return {};
}

} // namespace QtMultimediaTest
