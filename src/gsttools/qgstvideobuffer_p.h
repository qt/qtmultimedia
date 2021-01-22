/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QGSTVIDEOBUFFER_P_H
#define QGSTVIDEOBUFFER_P_H

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

#include <private/qgsttools_global_p.h>
#include <qabstractvideobuffer.h>
#include <QtCore/qvariant.h>

#include <gst/gst.h>
#include <gst/video/video.h>

QT_BEGIN_NAMESPACE

#if GST_CHECK_VERSION(1,0,0)
class Q_GSTTOOLS_EXPORT QGstVideoBuffer : public QAbstractPlanarVideoBuffer
{
public:
    QGstVideoBuffer(GstBuffer *buffer, const GstVideoInfo &info);
    QGstVideoBuffer(GstBuffer *buffer, const GstVideoInfo &info,
                    HandleType handleType, const QVariant &handle);
#else
class Q_GSTTOOLS_EXPORT QGstVideoBuffer : public QAbstractVideoBuffer
{
public:
    QGstVideoBuffer(GstBuffer *buffer, int bytesPerLine);
    QGstVideoBuffer(GstBuffer *buffer, int bytesPerLine,
                    HandleType handleType, const QVariant &handle);
#endif

    ~QGstVideoBuffer();

    GstBuffer *buffer() const { return m_buffer; }
    MapMode mapMode() const override;

#if GST_CHECK_VERSION(1,0,0)
    int map(MapMode mode, int *numBytes, int bytesPerLine[4], uchar *data[4]) override;
#else
    uchar *map(MapMode mode, int *numBytes, int *bytesPerLine) override;
#endif

    void unmap() override;

    QVariant handle() const override { return m_handle; }
private:
#if GST_CHECK_VERSION(1,0,0)
    GstVideoInfo m_videoInfo;
    GstVideoFrame m_frame;
#else
    int m_bytesPerLine = 0;
#endif
    GstBuffer *m_buffer = nullptr;
    MapMode m_mode = NotMapped;
    QVariant m_handle;
};

QT_END_NAMESPACE

#endif
