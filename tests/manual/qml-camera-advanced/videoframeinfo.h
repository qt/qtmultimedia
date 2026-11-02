// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef VIDEOFRAMEINFO_H
#define VIDEOFRAMEINFO_H

#include <QtCore/qobject.h>
#include <QtCore/qsize.h>
#include <QtCore/qstring.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideosink.h>
#include <QtQml/qqmlregistration.h>

// QML can't inspect a QVideoFrame itself.
class VideoFrameInfo
{
    Q_GADGET
    Q_PROPERTY(bool valid MEMBER valid CONSTANT FINAL)
    // In microseconds, or -1 if the frame has no timestamp.
    Q_PROPERTY(qint64 startTime MEMBER startTime CONSTANT FINAL)
    Q_PROPERTY(qint64 endTime MEMBER endTime CONSTANT FINAL)
    Q_PROPERTY(QSize resolution MEMBER resolution CONSTANT FINAL)
    Q_PROPERTY(QString pixelFormat MEMBER pixelFormat CONSTANT FINAL)
    Q_PROPERTY(QString rotation MEMBER rotation CONSTANT FINAL)
    Q_PROPERTY(bool mirrored MEMBER mirrored CONSTANT FINAL)
    // The frame's own rotation and mirroring are applied on top of these.
    Q_PROPERTY(QString surfaceFormatRotation MEMBER surfaceFormatRotation CONSTANT FINAL)
    Q_PROPERTY(bool surfaceFormatMirrored MEMBER surfaceFormatMirrored CONSTANT FINAL)
    Q_PROPERTY(QString colorSpace MEMBER colorSpace CONSTANT FINAL)
    Q_PROPERTY(QString colorTransfer MEMBER colorTransfer CONSTANT FINAL)
    Q_PROPERTY(QString colorRange MEMBER colorRange CONSTANT FINAL)
    // QML value types must have lowercase names.
    QML_VALUE_TYPE(videoFrameInfo)
    QML_UNCREATABLE("videoFrameInfo is created by VideoFrameInspector.inspect()")

public:
    bool valid = false;
    qint64 startTime = -1;
    qint64 endTime = -1;
    QSize resolution;
    QString pixelFormat;
    QString rotation;
    bool mirrored = false;
    QString surfaceFormatRotation;
    bool surfaceFormatMirrored = false;
    QString colorSpace;
    QString colorTransfer;
    QString colorRange;
};

class VideoFrameInspector : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    [[nodiscard]] Q_INVOKABLE VideoFrameInfo inspect(const QVideoFrame &frame) const;

    // VideoOutput's sink emits videoFrameChanged() without the frame in QML.
    [[nodiscard]] Q_INVOKABLE VideoFrameInfo inspectLatest(QVideoSink *sink) const;
};

#endif // VIDEOFRAMEINFO_H
