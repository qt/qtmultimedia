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

#ifndef QGSTUTILS_P_H
#define QGSTUTILS_P_H

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
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qset.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <qaudioformat.h>
#include <qcamera.h>
#include <qabstractvideobuffer.h>
#include <qvideoframe.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

class QSize;
class QVariant;
class QByteArray;
class QImage;
class QVideoSurfaceFormat;

namespace QGstUtils {
    Q_MULTIMEDIA_EXPORT QSize capsResolution(const GstCaps *caps);
    Q_MULTIMEDIA_EXPORT QSize capsCorrectedResolution(const GstCaps *caps);
    Q_MULTIMEDIA_EXPORT QAudioFormat audioFormatForCaps(const GstCaps *caps);
    Q_MULTIMEDIA_EXPORT QAudioFormat audioFormatForSample(GstSample *sample);
    Q_MULTIMEDIA_EXPORT GstCaps *capsForAudioFormat(const QAudioFormat &format);
    Q_MULTIMEDIA_EXPORT void initializeGst();

    Q_MULTIMEDIA_EXPORT QImage bufferToImage(GstBuffer *buffer, const GstVideoInfo &info);
    Q_MULTIMEDIA_EXPORT QVideoSurfaceFormat formatForCaps(
            GstCaps *caps,
            GstVideoInfo *info = 0,
            QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle);

    Q_MULTIMEDIA_EXPORT GstCaps *capsForFormats(const QList<QVideoFrame::PixelFormat> &formats);
    void setFrameTimeStamps(QVideoFrame *frame, GstBuffer *buffer);

    Q_MULTIMEDIA_EXPORT void setMetaData(GstElement *element, const QMap<QByteArray, QVariant> &data);
    Q_MULTIMEDIA_EXPORT void setMetaData(GstBin *bin, const QMap<QByteArray, QVariant> &data);

    Q_MULTIMEDIA_EXPORT GstCaps *videoFilterCaps();

    Q_MULTIMEDIA_EXPORT QVariant fromGStreamerOrientation(const QVariant &value);
    Q_MULTIMEDIA_EXPORT QVariant toGStreamerOrientation(const QVariant &value);

    Q_MULTIMEDIA_EXPORT bool useOpenGL();
}

template <typename T> struct QGRange
{
    T min;
    T max;
};

class QGValue
{
public:
    QGValue(const GValue *v) : value(v) {}
    const GValue *value;

    bool isNull() const { return !value; }

    std::optional<bool> toBool() const
    {
        if (!G_VALUE_HOLDS_BOOLEAN(value))
            return std::nullopt;
        return g_value_get_boolean(value);
    }
    std::optional<int> toInt() const
    {
        if (!G_VALUE_HOLDS_INT(value))
            return std::nullopt;
        return g_value_get_int(value);
    }
    const char *toString() const
    {
        return g_value_get_string(value);
    }
    std::optional<float> getFraction() const
    {
        if (!GST_VALUE_HOLDS_FRACTION(value))
            return std::nullopt;
        return (float)gst_value_get_fraction_numerator(value)/(float)gst_value_get_fraction_denominator(value);
    }

    std::optional<QGRange<float>> getFractionRange() const
    {
        if (!GST_VALUE_HOLDS_FRACTION_RANGE(value))
            return std::nullopt;
        QGValue min = gst_value_get_fraction_range_min(value);
        QGValue max = gst_value_get_fraction_range_max(value);
        return QGRange<float>{ *min.getFraction(), *max.getFraction() };
    }

    std::optional<QGRange<int>> toIntRange() const
    {
        if (!GST_VALUE_HOLDS_INT_RANGE(value))
            return std::nullopt;
        return QGRange<int>{ gst_value_get_int_range_min(value), gst_value_get_int_range_max(value) };
    }

    Q_MULTIMEDIA_EXPORT QList<QAudioFormat::SampleFormat> getSampleFormats() const;
};

class QGstStructure {
public:
    GstStructure *structure;
    QGstStructure(GstStructure *s) : structure(s) {}
    void free() { gst_structure_free(structure); structure = nullptr; }

    bool isNull() const { return !structure; }

    QByteArray name() const { return gst_structure_get_name(structure); }

    QGValue operator[](const char *name) const { return gst_structure_get_value(structure, name); }

    Q_MULTIMEDIA_EXPORT QSize resolution() const;
    Q_MULTIMEDIA_EXPORT QVideoFrame::PixelFormat pixelFormat() const;
    Q_MULTIMEDIA_EXPORT QSize pixelAspectRatio() const;
    Q_MULTIMEDIA_EXPORT QGRange<float> frameRateRange() const;

    QByteArray toString() const { return gst_structure_to_string(structure); }
};

class QGstCaps {
    const GstCaps *caps;
public:
    QGstCaps(const GstCaps *c) : caps(c) {}

    bool isNull() const { return !caps; }

    int size() const { return gst_caps_get_size(caps); }
    QGstStructure at(int index) { return gst_caps_get_structure(caps, index); }
    const GstCaps *get() const { return caps; }
};

class QGstMutableCaps {
    GstCaps *caps;
public:
    enum RefMode { HasRef, NeedsRef };
    QGstMutableCaps(GstCaps *c, RefMode mode = HasRef)
        : caps(c)
    {
        if (mode == NeedsRef)
            gst_caps_ref(caps);
    }
    QGstMutableCaps(const QGstMutableCaps &other)
        : caps(other.caps)
    {
        if (caps)
            gst_caps_ref(caps);
    }
    QGstMutableCaps &operator=(const QGstMutableCaps &other)
    {
        if (other.caps)
            gst_caps_ref(other.caps);
        if (caps)
            gst_caps_unref(const_cast<GstCaps *>(caps));
        caps = other.caps;
        return *this;
    }
    ~QGstMutableCaps() {
        if (caps)
            gst_caps_unref(const_cast<GstCaps *>(caps));
    }

    bool isNull() const { return !caps; }

    int size() const { return gst_caps_get_size(caps); }
    QGstStructure at(int index) { return gst_caps_get_structure(caps, index); }
    GstCaps *get() { return caps; }
};

Q_MULTIMEDIA_EXPORT const gchar *qt_gst_element_get_factory_name(GstElement *element);
Q_MULTIMEDIA_EXPORT GList *qt_gst_video_sinks();
QPair<int,int> qt_gstRateAsRational(qreal frameRate);

Q_MULTIMEDIA_EXPORT QDebug operator <<(QDebug debug, GstCaps *caps);

QT_END_NAMESPACE

#endif
