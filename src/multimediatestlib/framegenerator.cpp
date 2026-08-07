// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "framegenerator_p.h"

#include <QtMultimedia/private/qplatformmediaintegration_p.h>
#include <QtMultimedia/private/qmemoryvideobuffer_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>

#include <QtCore/qbuffer.h>

QT_BEGIN_NAMESPACE

void VideoGenerator::setPattern(ImagePattern pattern)
{
    m_pattern = pattern;
}

void VideoGenerator::setFrameCount(int count)
{
    m_maxFrameCount = count;
}

void VideoGenerator::setSize(QSize size)
{
    m_size = size;
}

void VideoGenerator::setPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat)
{
    m_pixelFormat = pixelFormat;
}

void VideoGenerator::setFrameRate(double rate)
{
    m_frameRate = rate;
}

void VideoGenerator::setPeriod(std::chrono::microseconds period)
{
    m_period = period;
}

void VideoGenerator::setPresentationRotation(QtVideo::Rotation rotation)
{
    m_presentationRotation = rotation;
}

void VideoGenerator::setPresentationMirrored(bool mirror)
{
    m_presentationMirrored = mirror;
}
void VideoGenerator::setEndTimeReporting(bool endTimeIsReported)
{
    m_endTimeIsReported = endTimeIsReported;
}

void VideoGenerator::setColors(std::array<QColor, 5> newColors)
{
    m_colors = newColors;
}

void VideoGenerator::emitEmptyFrameOnStop()
{
    m_emitEmptyFrameOnStop = true;
}

static void fillColoredSquares(QImage& image)
{
    QList<QColor> colors = { Qt::red, Qt::green, Qt::blue, Qt::yellow };
    const int width = image.width();
    const int height = image.height();

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            const int colorX = i < width / 2 ? 0 : 1;
            const int colorY = j < height / 2 ? 0 : 1;
            const int colorIndex = colorX + 2 * colorY;
            image.setPixel(i, j, colors[colorIndex].rgb());
        }
    }
}

static QVideoFrame createJpegFrame(const QImage &image, QSize size)
{
    QByteArray jpegData;
    QBuffer buffer(&jpegData);
    if (!buffer.open(QIODevice::WriteOnly))
        return {};

    // Maximum quality, so that the comparison against the source image stays meaningful
    if (!image.save(&buffer, "JPG", 100))
        return {};

    // JPEG frames hold the compressed data in a single plane without a line stride
    auto videoBuffer = std::make_unique<QMemoryVideoBuffer>(std::move(jpegData), -1);
    return QVideoFramePrivate::createFrame(std::move(videoBuffer),
                                           QVideoFrameFormat(size, QVideoFrameFormat::Format_Jpeg));
}

QVideoFrame VideoGenerator::createFrame()
{
    using namespace std::chrono;

    QImage image(m_size, QImage::Format_ARGB32);
    switch (m_pattern) {
    case ImagePattern::SingleColor:
        image.fill(m_colors[m_frameIndex % m_colors.size()]);
        break;
    case ImagePattern::ColoredSquares:
        fillColoredSquares(image);
        break;
    }

    QVideoFrame frame;
    if (m_pixelFormat == QVideoFrameFormat::Format_Jpeg) {
        frame = createJpegFrame(image, m_size);
    } else {
        QVideoFrame rgbFrame(image);
        QVideoFrameFormat outputFormat{ m_size, m_pixelFormat };
        frame = QPlatformMediaIntegration::instance()->convertVideoFrame(rgbFrame, outputFormat);
    }

    if (m_frameRate)
        frame.setStreamFrameRate(*m_frameRate);

    if (m_period) {
        frame.setStartTime(m_period->count() * m_frameIndex);
        if (m_endTimeIsReported)
            frame.setEndTime(m_period->count() * (m_frameIndex + 1));
    }

    if (m_presentationRotation)
        frame.setRotation(*m_presentationRotation);

    if (m_presentationMirrored)
        frame.setMirrored(*m_presentationMirrored);

    return frame;
}

void VideoGenerator::nextFrame()
{
    if (m_frameIndex == m_maxFrameCount) {
        emit done();
        if (m_emitEmptyFrameOnStop)
            emit frameCreated({});
        return;
    }

    const QVideoFrame frame = createFrame();
    emit frameCreated(frame);
    ++m_frameIndex;
}

QT_END_NAMESPACE

#include "moc_framegenerator_p.cpp"
