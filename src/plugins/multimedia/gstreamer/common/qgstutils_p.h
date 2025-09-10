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

#include <gst/gstsample.h>
#include <gst/gstbuffer.h>

#include <QtCore/qglobal.h>
#include <QtCore/qlocale.h>

QT_BEGIN_NAMESPACE

class QAudioFormat;
class QGstCaps;
class QVideoFrame;

namespace QGstUtils {
QAudioFormat audioFormatForSample(GstSample *sample);
QAudioFormat audioFormatForCaps(const QGstCaps &caps);
QGstCaps capsForAudioFormat(const QAudioFormat &format);

void setFrameTimeStampsFromBuffer(QVideoFrame *frame, GstBuffer *buffer);

QLocale::Language codeToLanguage(const gchar *);
} // namespace QGstUtils

GList *qt_gst_video_sinks();

QT_END_NAMESPACE

#endif
