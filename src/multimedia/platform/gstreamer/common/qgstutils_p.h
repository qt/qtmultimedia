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
#include <private/qgst_p.h>
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

Q_MULTIMEDIA_EXPORT const gchar *qt_gst_element_get_factory_name(GstElement *element);
Q_MULTIMEDIA_EXPORT GList *qt_gst_video_sinks();
QPair<int,int> qt_gstRateAsRational(qreal frameRate);

Q_MULTIMEDIA_EXPORT QDebug operator <<(QDebug debug, GstCaps *caps);

QT_END_NAMESPACE

#endif
