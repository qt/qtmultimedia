// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidcameraframe_p.h"
#include <jni.h>
#include <QDebug>
#include <QtCore/QLoggingCategory>

Q_DECLARE_JNI_CLASS(AndroidImageFormat, "android/graphics/ImageFormat");

Q_DECLARE_JNI_TYPE(AndroidImage, "Landroid/media/Image;")
Q_DECLARE_JNI_TYPE(AndroidImagePlaneArray, "[Landroid/media/Image$Plane;")
Q_DECLARE_JNI_TYPE(JavaByteBuffer, "Ljava/nio/ByteBuffer;")

QT_BEGIN_NAMESPACE
static Q_LOGGING_CATEGORY(qLCAndroidCameraFrame, "qt.multimedia.ffmpeg.android.camera.frame");

bool QAndroidCameraFrame::parse(const QJniObject &frame)
{
    QJniEnvironment jniEnv;

    if (!frame.isValid())
        return false;

    auto planes = frame.callMethod<QtJniTypes::AndroidImagePlaneArray>("getPlanes");
    if (!planes.isValid())
        return false;

    int numberPlanes = jniEnv->GetArrayLength(planes.object<jarray>());
    // create and populate temporary array structure
    int pixelStrides[numberPlanes];
    int rowStrides[numberPlanes];
    int bufferSize[numberPlanes];
    uint8_t *buffer[numberPlanes];

    auto resetPlane = [&](int index) {
        if (index < 0 || index > numberPlanes)
            return;

        rowStrides[index] = 0;
        pixelStrides[index] = 0;
        bufferSize[index] = 0;
        buffer[index] = nullptr;
    };

    for (int index = 0; index < numberPlanes; index++) {
        QJniObject plane = jniEnv->GetObjectArrayElement(planes.object<jobjectArray>(), index);
        if (jniEnv.checkAndClearExceptions() || !plane.isValid()) {
            resetPlane(index);
            continue;
        }

        rowStrides[index] = plane.callMethod<jint>("getRowStride");
        pixelStrides[index] = plane.callMethod<jint>("getPixelStride");

        auto byteBuffer = plane.callMethod<QtJniTypes::JavaByteBuffer>("getBuffer");
        if (!byteBuffer.isValid()) {
            resetPlane(index);
            continue;
        }

        // Uses direct access which is garanteed by android to work with
        // ImageReader bytebuffer
        buffer[index] = static_cast<uint8_t *>(jniEnv->GetDirectBufferAddress(byteBuffer.object()));
        bufferSize[index] = byteBuffer.callMethod<jint>("remaining");
    }

    QVideoFrameFormat::PixelFormat calculedPixelFormat = QVideoFrameFormat::Format_Invalid;

    // finding the image format
    // the ImageFormats that can happen here are stated here:
    // https://developer.android.com/reference/android/media/Image#getFormat()
    int format = frame.callMethod<jint>("getFormat");
    AndroidImageFormat imageFormat = AndroidImageFormat(format);

    switch (imageFormat) {
    case AndroidImageFormat::JPEG:
        calculedPixelFormat = QVideoFrameFormat::Format_Jpeg;
        break;
    case AndroidImageFormat::YUV_420_888:
        if (numberPlanes < 3) {
            // something went wrong on parsing. YUV_420_888 format must always have 3 planes
            calculedPixelFormat = QVideoFrameFormat::Format_Invalid;
            break;
        }
        if (pixelStrides[1] == 1)
            calculedPixelFormat = QVideoFrameFormat::Format_YUV420P;
        else if (pixelStrides[1] == 2 && abs(buffer[1] - buffer[2]) == 1)
            // this can be NV21, but it will converted below
            calculedPixelFormat = QVideoFrameFormat::Format_NV12;
        break;
    case AndroidImageFormat::HEIC:
        // QImage cannot parse HEIC
        calculedPixelFormat = QVideoFrameFormat::Format_Invalid;
        break;
    case AndroidImageFormat::RAW_PRIVATE:
    case AndroidImageFormat::RAW_SENSOR:
        // we cannot know raw formats
        calculedPixelFormat = QVideoFrameFormat::Format_Invalid;
        break;
    case AndroidImageFormat::FLEX_RGBA_8888:
    case AndroidImageFormat::FLEX_RGB_888:
        // these formats are only returned by Mediacodec.getOutputImage, they are not used as a
        // Camera2 Image frame return
        calculedPixelFormat = QVideoFrameFormat::Format_Invalid;
        break;
    case AndroidImageFormat::YUV_422_888:
    case AndroidImageFormat::YUV_444_888:
    case AndroidImageFormat::YCBCR_P010:
        // not dealing with these formats, they require higher API levels than the current Qt min
        calculedPixelFormat = QVideoFrameFormat::Format_Invalid;
        break;
    default:
        calculedPixelFormat = QVideoFrameFormat::Format_Invalid;
        break;
    }

    if (calculedPixelFormat == QVideoFrameFormat::Format_Invalid) {
        qCWarning(qLCAndroidCameraFrame) << "Cannot determine image format!";
        return false;
    }

    auto copyPlane = [&](int mapIndex, int arrayIndex) {
        if (arrayIndex >= numberPlanes)
            return;

        m_planes[mapIndex].rowStride = rowStrides[arrayIndex];
        m_planes[mapIndex].size = bufferSize[arrayIndex];
        m_planes[mapIndex].data = buffer[arrayIndex];
    };

    switch (calculedPixelFormat) {
    case QVideoFrameFormat::Format_YUV420P:
        m_numberPlanes = 3;
        copyPlane(0, 0);
        copyPlane(1, 1);
        copyPlane(2, 2);
        m_pixelFormat = QVideoFrameFormat::Format_YUV420P;
        break;
    case QVideoFrameFormat::Format_NV12:
        m_numberPlanes = 2;
        copyPlane(0, 0);
        copyPlane(1, 1);
        m_pixelFormat = QVideoFrameFormat::Format_NV12;
        break;
    case QVideoFrameFormat::Format_Jpeg:
        qCWarning(qLCAndroidCameraFrame)
                << "FFMpeg HW Mediacodec does not encode other than YCbCr formats";
        // we still parse it to preview the frame
        m_image = QImage::fromData(buffer[0], bufferSize[0]);
        m_planes[0].rowStride = m_image.bytesPerLine();
        m_planes[0].size = m_image.sizeInBytes();
        m_planes[0].data = m_image.bits();
        m_pixelFormat = QVideoFrameFormat::pixelFormatFromImageFormat(m_image.format());
        break;
    default:
        break;
    }

    long timestamp = frame.callMethod<jlong>("getTimestamp");
    m_timestamp = timestamp / 1000;

    int width = frame.callMethod<jint>("getWidth");
    int height = frame.callMethod<jint>("getHeight");
    m_size = QSize(width, height);

    return true;
}

QAndroidCameraFrame::QAndroidCameraFrame(QJniObject frame)
    : m_pixelFormat(QVideoFrameFormat::Format_Invalid), m_parsed(parse(frame))
{
    if (isParsed()) {
        // holding the frame java object
        QJniEnvironment jniEnv;
        m_frame = jniEnv->NewGlobalRef(frame.object());
        jniEnv.checkAndClearExceptions();
    } else if (frame.isValid()) {
        frame.callMethod<void>("close");
    }
}

QAndroidCameraFrame::~QAndroidCameraFrame()
{
    if (!isParsed()) // nothing to clean
        return;

    QJniObject qFrame(m_frame);
    if (qFrame.isValid())
        qFrame.callMethod<void>("close");

    QJniEnvironment jniEnv;
    if (m_frame)
        jniEnv->DeleteGlobalRef(m_frame);
}

QT_END_NAMESPACE
