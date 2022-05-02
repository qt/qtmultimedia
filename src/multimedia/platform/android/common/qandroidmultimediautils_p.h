/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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

bool qt_androidRequestCameraPermission();
bool qt_androidRequestRecordingPermission();
bool qt_androidRequestWriteStoragePermission();

bool qt_androidCheckCameraPermission();
bool qt_androidCheckMicrophonePermission();

QT_END_NAMESPACE

#endif // QANDROIDMULTIMEDIAUTILS_H
