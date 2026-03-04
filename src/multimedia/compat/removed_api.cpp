// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define QT_MULTIMEDIA_BUILD_REMOVED_API

#include <QtMultimedia/qtmultimediaglobal.h>

#if QT_MULTIMEDIA_REMOVED_SINCE(6, 10) && !defined(Q_QDOC)

#  include "qaudio.h"

QT_BEGIN_NAMESPACE

namespace QAudio {
float convertVolume(float volume, VolumeScale from, VolumeScale to)
{
    return QtAudio::convertVolume(volume, from, to);
}
} // namespace QAudio

QT_END_NAMESPACE

#endif // QT_MULTIMEDIA_REMOVED_SINCE(6, 10) && !defined(Q_QDOC)

#if QT_MULTIMEDIA_REMOVED_SINCE(6, 11) && !defined(Q_QDOC)

#  include <QtMultimedia/qvideoframeformat.h>
#  include <QtMultimedia/private/qmultimedia_enum_to_string_converter_p.h>

QT_BEGIN_NAMESPACE

// clang-format off

#  ifndef QT_NO_DEBUG_STREAM
#     if QT_DEPRECATED_SINCE(6, 4)
QT_MM_MAKE_STRING_RESOLVER(QVideoFrameFormat::YCbCrColorSpace, QtMultimediaPrivate::EnumName,
                           (QVideoFrameFormat::YCbCr_Undefined, "YCbCr_Undefined")
                           (QVideoFrameFormat::YCbCr_BT601,     "YCbCr_BT601")
                           (QVideoFrameFormat::YCbCr_BT709,     "YCbCr_BT709")
                           (QVideoFrameFormat::YCbCr_xvYCC601,  "YCbCr_xvYCC601")
                           (QVideoFrameFormat::YCbCr_xvYCC709,  "YCbCr_xvYCC709")
                           (QVideoFrameFormat::YCbCr_JPEG,      "YCbCr_JPEG")
                           (QVideoFrameFormat::YCbCr_BT2020,    "YCbCr_BT2020")
                           );
QT_MM_DEFINE_QDEBUG_ENUM(QVideoFrameFormat::YCbCrColorSpace);
#     endif // QT_DEPRECATED_SINCE(6, 4)

QT_MM_MAKE_STRING_RESOLVER(QVideoFrameFormat::ColorSpace, QtMultimediaPrivate::EnumName,
                           (QVideoFrameFormat::ColorSpace_BT601,     "ColorSpace_BT601")
                           (QVideoFrameFormat::ColorSpace_BT709,     "ColorSpace_BT709")
                           (QVideoFrameFormat::ColorSpace_AdobeRgb,  "ColorSpace_AdobeRgb")
                           (QVideoFrameFormat::ColorSpace_BT2020,    "ColorSpace_BT2020")
                           (QVideoFrameFormat::ColorSpace_Undefined, "ColorSpace_Undefined")
                           );
QT_MM_DEFINE_QDEBUG_ENUM(QVideoFrameFormat::ColorSpace);

QT_MM_MAKE_STRING_RESOLVER(QVideoFrameFormat::Direction, QtMultimediaPrivate::EnumName,
                           (QVideoFrameFormat::TopToBottom, "TopToBottom")
                           (QVideoFrameFormat::BottomToTop, "BottomToTop")
                           );
QT_MM_DEFINE_QDEBUG_ENUM(QVideoFrameFormat::Direction);

#  endif // QT_NO_DEBUG_STREAM

// clang-format off

QT_END_NAMESPACE

#endif // QT_MULTIMEDIA_REMOVED_SINCE(6, 11) && !defined(Q_QDOC)
