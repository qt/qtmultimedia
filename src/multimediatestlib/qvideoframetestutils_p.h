// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QVIDEOFRAMETESTUTILS_P_H
#define QVIDEOFRAMETESTUTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbytearray.h>
#include <QtCore/qbytearrayview.h>
#include <QtCore/qstring.h>
#include <QtCore/qtypes.h>
#include <QtCore/private/qexpected_p.h>

#include <initializer_list>

namespace QtMultimediaTest {

// Builds an owned byte pattern from a brace-init list of byte values, e.g. a multi-byte texel:
// makeBytes({ 0x12, 0x34 }).
QByteArray makeBytes(std::initializer_list<uchar> bytes);

// Fills "height" rows of "width" texels each, starting at "data" and advancing by "stride" bytes
// per row, by repeating "texel". Used to build known-content planar image data (e.g. a video
// frame plane) independently of how that data is ultimately produced (CPU buffer, GPU readback,
// FFmpeg AVFrame plane).
void fillPlaneRows(uchar *data, qsizetype stride, int width, int height, QByteArrayView texel);

// Compares "height" rows of "width" texels each, starting at "data" and advancing by "stride"
// bytes per row, against a repetition of "texel". Returns an error describing the first
// mismatch, if any.
q23::expected<void, QString> comparePlaneRows(const uchar *data, qsizetype stride, int width,
                                              int height, QByteArrayView texel);

} // namespace QtMultimediaTest

#endif // QVIDEOFRAMETESTUTILS_P_H
