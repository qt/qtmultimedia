// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidvideoframebuffer_p.h"
#include <jni.h>
#include <QDebug>
#include <QtCore/qjnitypes.h>
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE
Q_STATIC_LOGGING_CATEGORY(qLCAndroidCameraFrame, "qt.multimedia.ffmpeg.android.camera.frame");

namespace {
bool isWorkaroundForEmulatorNeeded() {
    const static bool workaroundForEmulator
                 = QtJniTypes::QtVideoDeviceManager::callStaticMethod<jboolean>("isEmulator");
    return workaroundForEmulator;
}
}

bool QAndroidVideoFrameBuffer::useCopiedData() const
{
    return m_policy == MemoryPolicy::Copy;
}

bool QAndroidVideoFrameBuffer::parse(const QJniObject &frame)
{
    QJniEnvironment jniEnv;

    if (!frame.isValid())
        return false;

    const auto planes = frame.callMethod<QtJniTypes::ImagePlane[]>("getPlanes");
    if (!planes.isValid())
        return false;

    const int numberPlanes = planes.size();
    Q_ASSERT(numberPlanes <= MAX_PLANES);

    // create and populate temporary array structure
    int pixelStrides[MAX_PLANES];
    int rowStrides[MAX_PLANES];
    int bufferSize[MAX_PLANES];
    char *buffer[MAX_PLANES];

    auto resetPlane = [&](int index) {
        if (index < 0 || index > numberPlanes)
            return;

        rowStrides[index] = 0;
        pixelStrides[index] = 0;
        bufferSize[index] = 0;
        buffer[index] = nullptr;
    };

    for (qsizetype index = 0; index < numberPlanes; ++index) {
        auto plane = planes.at(index);
        if (!plane.isValid()) {
            resetPlane(index);
            continue;
        }

        rowStrides[index] = plane.callMethod<jint>("getRowStride");
        pixelStrides[index] = plane.callMethod<jint>("getPixelStride");

        auto byteBuffer = plane.callMethod<QtJniTypes::ByteBuffer>("getBuffer");
        if (!byteBuffer.isValid()) {
            resetPlane(index);
            continue;
        }

        // Uses direct access which is garanteed by android to work with
        // ImageReader bytebuffer
        buffer[index] = static_cast<char *>(jniEnv->GetDirectBufferAddress(byteBuffer.object()));
        bufferSize[index] = byteBuffer.callMethod<jint>("remaining");
    }

    QVideoFrameFormat::PixelFormat calculedPixelFormat = QVideoFrameFormat::Format_Invalid;

    // finding the image format
    // the ImageFormats that can happen here are stated here:
    // https://developer.android.com/reference/android/media/Image#getFormat()
    int format = frame.callMethod<jint>("getFormat");
    AndroidImageFormat imageFormat = AndroidImageFormat(format);

    switch (imageFormat) {
    case AndroidImageFormat::RGBA_8888: {
        // RGBA_8888 should have a single plane
        calculedPixelFormat = numberPlanes != 1 ? QVideoFrameFormat::Format_Invalid :
                                                  QVideoFrameFormat::Format_RGBA8888;
        break;
    }
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
        else if (pixelStrides[1] == 2) {
            if (buffer[1] - buffer[2] == -1) // Interleaved UVUV -> NV12
                calculedPixelFormat = QVideoFrameFormat::Format_NV12;
            else if (buffer[1] - buffer[2] == 1) // Interleaved VUVU -> NV21
                calculedPixelFormat = QVideoFrameFormat::Format_NV21;
        }
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

        m_mapData.bytesPerLine[mapIndex] = rowStrides[arrayIndex];
        dataCleaner[mapIndex] = useCopiedData() ?
            QByteArray(buffer[arrayIndex], bufferSize[arrayIndex])
          : QByteArray::fromRawData(buffer[arrayIndex], bufferSize[arrayIndex]);
        m_mapData.data[mapIndex] = (uchar *)dataCleaner[mapIndex].constData();
        m_mapData.dataSize[mapIndex] = bufferSize[arrayIndex];
    };

    int width = frame.callMethod<jint>("getWidth");
    int height = frame.callMethod<jint>("getHeight");
    QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;

    switch (calculedPixelFormat) {
    case QVideoFrameFormat::Format_RGBA8888:
        m_mapData.planeCount = 1;
        if (isWorkaroundForEmulatorNeeded())
            m_policy = MemoryPolicy::Copy;

        copyPlane(0, 0);

        pixelFormat = QVideoFrameFormat::Format_RGBA8888;
        break;
    case QVideoFrameFormat::Format_YUV420P:
        m_mapData.planeCount = 3;
        if (isWorkaroundForEmulatorNeeded())
            m_policy = MemoryPolicy::Copy;
        copyPlane(0, 0);
        copyPlane(1, 1);
        copyPlane(2, 2);

        pixelFormat = QVideoFrameFormat::Format_YUV420P;
        break;
    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_NV21:
        // Y-plane and combined interleaved UV-plane
        m_mapData.planeCount = 2;
        copyPlane(0, 0);

        // Android reports U and V planes as planes[1] and planes[2] respectively, regardless of the
        // order of interleaved samples. We point to whichever is first in memory.
        // With interleaved UV plane, Android reports the size of each plane as the smallest size
        // that includes all samples of that plane. For example, if the UV plane is [u, v, u, v],
        // the size of the U-plane is 3, not 4. With FFmpeg we need to count the total number of
        // bytes in the UV-plane, which is 1 more than what Android reports.
        {
            const int indexOfFirstPlane = calculedPixelFormat == QVideoFrameFormat::Format_NV21 ?
                                          2 : 1;
            m_mapData.bytesPerLine[1] = rowStrides[indexOfFirstPlane];

            dataCleaner[1] = useCopiedData() ?
                                 QByteArray(buffer[indexOfFirstPlane],
                                            bufferSize[indexOfFirstPlane] + 1)
                               : QByteArray::fromRawData(buffer[indexOfFirstPlane],
                                            bufferSize[indexOfFirstPlane] + 1);
            m_mapData.data[1] = (uchar *)dataCleaner[1].constData();
            m_mapData.dataSize[1] = bufferSize[indexOfFirstPlane] + 1;
        }
        pixelFormat = calculedPixelFormat;
        break;
    case QVideoFrameFormat::Format_Jpeg:
        qCWarning(qLCAndroidCameraFrame)
                << "FFmpeg HW Mediacodec does not encode other than YCbCr formats";
        // we still parse it to preview the frame
        m_policy = MemoryPolicy::Copy;
        m_image = QImage::fromData((uchar *)buffer[0], bufferSize[0]);
        m_mapData.bytesPerLine[0] = m_image.bytesPerLine();
        dataCleaner[0] = QByteArray::fromRawData((char*)m_image.bits(), m_image.sizeInBytes());
        m_mapData.data[0] = (uchar *)dataCleaner[0].constData();
        m_mapData.dataSize[0] = m_image.sizeInBytes();
        pixelFormat = QVideoFrameFormat::pixelFormatFromImageFormat(m_image.format());
        break;
    default:
        break;
    }

    long timestamp = frame.callMethod<jlong>("getTimestamp");
    m_timestamp = timestamp / 1000;

    m_videoFrameFormat = QVideoFrameFormat(QSize(width, height), pixelFormat);
    return true;
}

QAndroidVideoFrameBuffer::QAndroidVideoFrameBuffer(QJniObject frame,
                        std::shared_ptr<FrameReleaseDelegate> frameReleaseDelegate,
                        MemoryPolicy policy,
                        QtVideo::Rotation rotation)
    : m_frameReleaseDelegate(frameReleaseDelegate)
    , m_policy(policy)
    , m_parsed(parse(frame))
{
    if (isParsed() && !useCopiedData()) {
        // holding the frame java object
        QJniEnvironment jniEnv;
        m_nativeFrame = jniEnv->NewGlobalRef(frame.object());
        jniEnv.checkAndClearExceptions();
    } else if (frame.isValid()) {
        frame.callMethod<void>("close");
        m_frameReleaseDelegate->onFrameReleased();
    }
    m_videoFrameFormat.setRotation(rotation);
}

QAndroidVideoFrameBuffer::~QAndroidVideoFrameBuffer()
{
    if (!isParsed() || useCopiedData()) // nothing to clean
        return;

    QJniObject qFrame(m_nativeFrame);
    if (qFrame.isValid()) {
        qFrame.callMethod<void>("close");
        m_frameReleaseDelegate->onFrameReleased();
    }

    QJniEnvironment jniEnv;
    if (m_nativeFrame)
        jniEnv->DeleteGlobalRef(m_nativeFrame);
}

QT_END_NAMESPACE
