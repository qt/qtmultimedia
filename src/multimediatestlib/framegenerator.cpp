// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "framegenerator_p.h"
#include <QtCore/qdebug.h>

#include <private/qplatformmediaintegration_p.h>

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

void VideoGenerator::setPeriod(std::chrono::milliseconds period)
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

QVideoFrame VideoGenerator::createFrame()
{
    using namespace std::chrono;

    QImage image(m_size, QImage::Format_ARGB32);
    switch (m_pattern) {
    case ImagePattern::SingleColor:
        image.fill(colors[m_frameIndex % colors.size()]);
        break;
    case ImagePattern::ColoredSquares:
        fillColoredSquares(image);
        break;
    }

    QVideoFrame rgbFrame(image);
    QVideoFrameFormat outputFormat { m_size, m_pixelFormat };
    QVideoFrame frame =
            QPlatformMediaIntegration::instance()->convertVideoFrame(rgbFrame, outputFormat);

    if (m_frameRate)
        frame.setStreamFrameRate(*m_frameRate);

    if (m_period) {
        frame.setStartTime(duration_cast<microseconds>(*m_period).count() * m_frameIndex);
        frame.setEndTime(duration_cast<microseconds>(*m_period).count() * (m_frameIndex + 1));
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
