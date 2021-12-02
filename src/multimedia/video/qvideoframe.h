/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QVIDEOFRAME_H
#define QVIDEOFRAME_H

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qvideoframeformat.h>

#include <QtCore/qmetatype.h>
#include <QtCore/qshareddata.h>
#include <QtGui/qimage.h>

QT_BEGIN_NAMESPACE

class QSize;
class QVideoFramePrivate;
class QAbstractVideoBuffer;
class QRhi;
class QRhiResourceUpdateBatch;
class QRhiTexture;

QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(QVideoFramePrivate, Q_MULTIMEDIA_EXPORT)

class Q_MULTIMEDIA_EXPORT QVideoFrame
{
public:

    enum HandleType
    {
        NoHandle,
        RhiTextureHandle
    };

    enum MapMode
    {
        NotMapped = 0x00,
        ReadOnly  = 0x01,
        WriteOnly = 0x02,
        ReadWrite = ReadOnly | WriteOnly
    };

    enum RotationAngle
    {
        Rotation0 = 0,
        Rotation90 = 90,
        Rotation180 = 180,
        Rotation270 = 270
    };

    QVideoFrame();
    QVideoFrame(const QVideoFrameFormat &format);
    QVideoFrame(const QVideoFrame &other);
    ~QVideoFrame();

    QVideoFrame(QVideoFrame &&other) noexcept = default;
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QVideoFrame)
    void swap(QVideoFrame &other) noexcept
    { qSwap(d, other.d); }


    QVideoFrame &operator =(const QVideoFrame &other);
    bool operator==(const QVideoFrame &other) const;
    bool operator!=(const QVideoFrame &other) const;

    bool isValid() const;

    QVideoFrameFormat::PixelFormat pixelFormat() const;

    QVideoFrameFormat surfaceFormat() const;
    QVideoFrame::HandleType handleType() const;

    QSize size() const;
    int width() const;
    int height() const;

    bool isMapped() const;
    bool isReadable() const;
    bool isWritable() const;

    QVideoFrame::MapMode mapMode() const;

    bool map(QVideoFrame::MapMode mode);
    void unmap();

    int bytesPerLine(int plane) const;

    uchar *bits(int plane);
    const uchar *bits(int plane) const;
    int mappedBytes(int plane) const;
    int planeCount() const;

    quint64 textureHandle(int plane) const;

    qint64 startTime() const;
    void setStartTime(qint64 time);

    qint64 endTime() const;
    void setEndTime(qint64 time);

    void setRotationAngle(RotationAngle);
    RotationAngle rotationAngle() const;

    void setMirrored(bool);
    bool mirrored() const;

    QImage toImage() const;

    struct PaintOptions {
        QColor backgroundColor = Qt::transparent;
        Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio;
        enum PaintFlag {
            DontDrawSubtitles = 0x1
        };
        Q_DECLARE_FLAGS(PaintFlags, PaintFlag)
        PaintFlags paintFlags = {};
    };

    QString subtitleText() const;
    void setSubtitleText(const QString &text);

    void paint(QPainter *painter, const QRectF &rect, const PaintOptions &options);

    QVideoFrame(QAbstractVideoBuffer *buffer, const QVideoFrameFormat &format);

    QAbstractVideoBuffer *videoBuffer() const;
private:
    QExplicitlySharedDataPointer<QVideoFramePrivate> d;
};

Q_DECLARE_SHARED(QVideoFrame)

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, const QVideoFrame&);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QVideoFrame::HandleType);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QVideoFrame)

#endif

