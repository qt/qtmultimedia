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

#ifndef QWINDOWSMULTIMEDIATUTILS_P_H
#define QWINDOWSMULTIMEDIATUTILS_P_H

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

#include <private/qtmultimediaglobal_p.h>
#include <private/qplatformmediaformatinfo_p.h>
#include <qvideoframeformat.h>
#include <guiddef.h>

QT_BEGIN_NAMESPACE

namespace QWindowsMultimediaUtils {

    QVideoFrameFormat::PixelFormat pixelFormatFromMediaSubtype(const GUID &subtype);

    GUID videoFormatForCodec(QMediaFormat::VideoCodec codec);

    QMediaFormat::VideoCodec codecForVideoFormat(GUID format);

    GUID audioFormatForCodec(QMediaFormat::AudioCodec codec);

    QMediaFormat::AudioCodec codecForAudioFormat(GUID format);

    GUID containerForVideoFileFormat(QMediaFormat::FileFormat format);

    GUID containerForAudioFileFormat(QMediaFormat::FileFormat format);
}

QT_END_NAMESPACE

#endif
