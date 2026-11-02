// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "videoframeinfo.h"

#include <QtCore/qmetaobject.h>
#include <QtMultimedia/qvideoframeformat.h>

namespace {

template <typename Enum>
[[nodiscard]] QString enumName(Enum value)
{
    return QString::fromLatin1(QMetaEnum::fromType<Enum>().valueToKey(int(value)));
}

} // namespace

VideoFrameInfo VideoFrameInspector::inspect(const QVideoFrame &frame) const
{
    VideoFrameInfo info;
    info.valid = frame.isValid();
    if (!info.valid)
        return info;

    const QVideoFrameFormat format = frame.surfaceFormat();
    info.startTime = frame.startTime();
    info.endTime = frame.endTime();
    info.resolution = frame.size();
    info.pixelFormat = QVideoFrameFormat::pixelFormatToString(frame.pixelFormat());
    info.rotation = enumName(frame.rotation());
    info.mirrored = frame.mirrored();
    info.surfaceFormatRotation = enumName(format.rotation());
    info.surfaceFormatMirrored = format.isMirrored();
    info.colorSpace = enumName(format.colorSpace());
    info.colorTransfer = enumName(format.colorTransfer());
    info.colorRange = enumName(format.colorRange());
    return info;
}

VideoFrameInfo VideoFrameInspector::inspectLatest(QVideoSink *sink) const
{
    if (!sink)
        return {};
    return inspect(sink->videoFrame());
}
