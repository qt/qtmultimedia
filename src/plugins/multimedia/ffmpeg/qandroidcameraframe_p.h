// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDCAMERAFRAME_H
#define QANDROIDCAMERAFRAME_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QVideoFrameFormat>
#include <QJniObject>

class QAndroidCameraFrame
{
public:
    struct Plane
    {
        int pixelStride = 0;
        int rowStride = 0;
        int size = 0;
        uint8_t *data;
    };

    QAndroidCameraFrame(QJniObject frame);
    ~QAndroidCameraFrame();

    QVideoFrameFormat::PixelFormat format() const { return m_pixelFormat; }
    int numberPlanes() const { return m_numberPlanes; }
    Plane plane(int index) const
    {
        if (index < 0 || index > numberPlanes())
            return {};

        return m_planes[index];
    }
    QSize size() const { return m_size; }
    long timestamp() const { return m_timestamp; }

    bool isParsed() const { return m_parsed; }

private:
    bool parse(const QJniObject &frame);
    QVideoFrameFormat::PixelFormat m_pixelFormat;

    QSize m_size = {};
    long m_timestamp = 0;
    int m_numberPlanes = 0;
    Plane m_planes[3]; // 3 max number planes
    jobject m_frame = nullptr;
    bool m_parsed = false;
    QImage m_image;

    enum AndroidImageFormat {
        RAW_SENSOR = 32,
        YUV_420_888 = 35,
        RAW_PRIVATE = 36,
        YUV_422_888 = 39,
        YUV_444_888 = 40,
        FLEX_RGB_888 = 41,
        FLEX_RGBA_8888 = 42,
        YCBCR_P010 = 54,
        JPEG = 256,
        HEIC = 1212500294
    };
};

#endif // QANDROIDCAMERAFRAME_H
