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
    QAudioFormat audioFormatForCaps(QGstCaps caps);
    Q_MULTIMEDIA_EXPORT QGstMutableCaps capsForAudioFormat(const QAudioFormat &format);

    void setFrameTimeStamps(QVideoFrame *frame, GstBuffer *buffer);
}

Q_MULTIMEDIA_EXPORT GList *qt_gst_video_sinks();

QT_END_NAMESPACE

#endif
