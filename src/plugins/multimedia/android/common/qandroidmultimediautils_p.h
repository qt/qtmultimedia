// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDMULTIMEDIAUTILS_H
#define QANDROIDMULTIMEDIAUTILS_H

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

#include <qglobal.h>
#include <qsize.h>
#include "androidcamera_p.h"

QT_BEGIN_NAMESPACE

// return the index of the closest value to <value> in <list>
// (binary search)
int qt_findClosestValue(const QList<int> &list, int value);

bool qt_sizeLessThan(const QSize &s1, const QSize &s2);

QVideoFrameFormat::PixelFormat qt_pixelFormatFromAndroidImageFormat(AndroidCamera::ImageFormat f);
AndroidCamera::ImageFormat qt_androidImageFormatFromPixelFormat(QVideoFrameFormat::PixelFormat f);

bool qt_androidRequestWriteStoragePermission();

bool qt_androidCheckCameraPermission();
bool qt_androidCheckMicrophonePermission();

QT_END_NAMESPACE

#endif // QANDROIDMULTIMEDIAUTILS_H
