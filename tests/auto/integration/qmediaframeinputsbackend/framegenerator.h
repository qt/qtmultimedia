// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef FRAMEGENERATOR_H
#define FRAMEGENERATOR_H

#include <QtCore/qobject.h>
#include <QtCore/qlist.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qaudiobuffer.h>
#include "../shared/audiogenerationutils.h"

#include <functional>
#include <chrono>

QT_BEGIN_NAMESPACE

using namespace std::chrono;

enum class ImagePattern
{
    SingleColor, // Image filled with a single color
    ColoredSquares // Colored squares, [red, green; blue, yellow]
};

class VideoGenerator : public QObject
{
    Q_OBJECT
public:
    void setPattern(ImagePattern pattern);
    void setFrameCount(int count);
    void setSize(QSize size);
    void setPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat);
    void setFrameRate(double rate);
    void setPeriod(milliseconds period);
    void setPresentationRotation(QtVideo::Rotation rotation);
    void setPresentationMirrored(bool mirror);
    void emitEmptyFrameOnStop();
    QVideoFrame createFrame();

signals:
    void done();
    void frameCreated(const QVideoFrame &frame);

public slots:
    void nextFrame();

private:
    QList<QColor> colors = { Qt::red, Qt::green, Qt::blue, Qt::black, Qt::white };
    ImagePattern m_pattern = ImagePattern::SingleColor;
    QSize m_size{ 640, 480 };
    QVideoFrameFormat::PixelFormat m_pixelFormat = QVideoFrameFormat::Format_BGRA8888;
    std::optional<int> m_maxFrameCount;
    int m_frameIndex = 0;
    std::optional<double> m_frameRate;
    std::optional<milliseconds> m_period;
    std::optional<QtVideo::Rotation> m_presentationRotation;
    std::optional<bool> m_presentationMirrored;
    bool m_emitEmptyFrameOnStop = false;
};

QT_END_NAMESPACE

#endif
