// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include <qgst_p.h>
#include <gst/video/video.h>
#include <qaudioformat.h>
#include <qcamera.h>
#include <qvideoframe.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

class QSize;
class QVariant;
class QByteArray;
class QImage;
class QVideoFrameFormat;

namespace QGstUtils {
    Q_MULTIMEDIA_EXPORT QAudioFormat audioFormatForSample(GstSample *sample);
    QAudioFormat audioFormatForCaps(const QGstCaps &caps);
    Q_MULTIMEDIA_EXPORT QGstCaps capsForAudioFormat(const QAudioFormat &format);

    void setFrameTimeStamps(QVideoFrame *frame, GstBuffer *buffer);
}

Q_MULTIMEDIA_EXPORT GList *qt_gst_video_sinks();

QT_END_NAMESPACE

#endif
