// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDVIDEOFRAMEBUFFER_P_H
#define QANDROIDVIDEOFRAMEBUFFER_P_H

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

#include <QAbstractVideoBuffer>
#include <QByteArray>
#include <QVideoFrameFormat>
#include <QJniObject>
#include <QtCore/qjnitypes.h>

Q_DECLARE_JNI_CLASS(QtCamera2, "org/qtproject/qt/android/multimedia/QtCamera2")
Q_DECLARE_JNI_CLASS(QtVideoDeviceManager,
                    "org/qtproject/qt/android/multimedia/QtVideoDeviceManager")

Q_DECLARE_JNI_CLASS(Image, "android/media/Image")
Q_DECLARE_JNI_CLASS(ImageFormat, "android/graphics/ImageFormat")
Q_DECLARE_JNI_CLASS(ImagePlane, "android/media/Image$Plane")
Q_DECLARE_JNI_CLASS(ByteBuffer, "java/nio/ByteBuffer")

class QAndroidVideoFrameBuffer : public QAbstractVideoBuffer
{
public:
    class FrameReleaseDelegate {
        public:
            virtual ~FrameReleaseDelegate() = default;
            virtual void onFrameReleased() = 0;
    };
    enum class MemoryPolicy {
        Copy,   // Make a copy of frame data
        Reuse   // Reuse frame data
    };

    // Please note that MemoryPolicy::Reuse can be changed internally to MemoryPolicy::Copy
    QAndroidVideoFrameBuffer(QJniObject frame,
                        std::shared_ptr<FrameReleaseDelegate> frameReleaseDelegate,
                        MemoryPolicy policy,
                        QtVideo::Rotation rotation = QtVideo::Rotation::None);
    ~QAndroidVideoFrameBuffer();
    MapData map(QVideoFrame::MapMode) override { return m_mapData; }
    QVideoFrameFormat format() const override { return m_videoFrameFormat; }
    long timestamp() const { return m_timestamp; }
    bool isParsed() const { return m_parsed; }

private:
    bool useCopiedData() const;
    bool parse(const QJniObject &frame);
    QVideoFrameFormat m_videoFrameFormat;
    long m_timestamp = 0;
    MapData m_mapData;
    // Currently we have maximum 3 planes here
    // We keep this data in QByteArray for proper cleaning
    static constexpr int MAX_PLANES = 3;
    QByteArray dataCleaner[MAX_PLANES];
    jobject m_nativeFrame = nullptr;
    std::shared_ptr<FrameReleaseDelegate> m_frameReleaseDelegate;
    MemoryPolicy m_policy;
    bool m_parsed = false;
    QImage m_image;

    enum AndroidImageFormat {
    // Values taken from Android API ImageFormat, PixelFormat, or HardwareBuffer
    // (that can be retuned by Image::getFormat() method)
    // https://developer.android.com/reference/android/media/Image#getFormat()
        RGBA_8888 = 1,
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

#endif // QANDROIDVIDEOFRAMEBUFFER_P_H
