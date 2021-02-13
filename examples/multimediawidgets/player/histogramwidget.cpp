/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "histogramwidget.h"
#include <QPainter>
#include <QHBoxLayout>

class QAudioLevel : public QWidget
{
    Q_OBJECT
public:
    explicit QAudioLevel(QWidget *parent = nullptr);

    // Using [0; 1.0] range
    void setLevel(qreal level);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    qreal m_level = 0;
};

QAudioLevel::QAudioLevel(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(15);
    setMaximumHeight(50);
}

void QAudioLevel::setLevel(qreal level)
{
    if (m_level != level) {
        m_level = level;
        update();
    }
}

void QAudioLevel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    // draw level
    qreal widthLevel = m_level * width();
    painter.fillRect(0, 0, widthLevel, height(), Qt::red);
    // clear the rest of the control
    painter.fillRect(widthLevel, 0, width(), height(), Qt::black);
}

HistogramWidget::HistogramWidget(QWidget *parent)
    : QWidget(parent)
{
    m_processor.moveToThread(&m_processorThread);
    connect(&m_processor, &FrameProcessor::histogramReady, this, &HistogramWidget::setHistogram);
    m_processorThread.start(QThread::LowestPriority);
    setLayout(new QHBoxLayout);
}

HistogramWidget::~HistogramWidget()
{
    m_processorThread.quit();
    m_processorThread.wait(10000);
}

void HistogramWidget::processFrame(const QVideoFrame &frame)
{
    if (m_isBusy && frame.isValid())
        return; //drop frame

    m_isBusy = true;
    QMetaObject::invokeMethod(&m_processor, "processFrame",
                              Qt::QueuedConnection, Q_ARG(QVideoFrame, frame), Q_ARG(int, m_levels));
}

// returns the audio level for each channel
QList<qreal> getBufferLevels(const QAudioBuffer &buffer)
{
    QList<qreal> values;

    if (!buffer.isValid())
        return values;

    if (!buffer.format().isValid())
        return values;

    auto format = buffer.format();
    int channelCount = format.channelCount();
    int bytesPerSample = format.bytesPerFrame();
    int frames = buffer.frameCount();

    values.fill(0, channelCount);

    const char *data = buffer.constData<char>();
    for (int i = 0; i < frames; ++i) {
        for (int j = 0; j < channelCount; ++j) {
            qreal value = format.normalizedSampleValue(data);
            if (value > values.at(j))
                values[j] = value;
            data += bytesPerSample;
        }
    }

    return values;
}

void HistogramWidget::processBuffer(const QAudioBuffer &buffer)
{
    if (m_audioLevels.count() != buffer.format().channelCount()) {
        qDeleteAll(m_audioLevels);
        m_audioLevels.clear();
        for (int i = 0; i < buffer.format().channelCount(); ++i) {
            QAudioLevel *level = new QAudioLevel(this);
            m_audioLevels.append(level);
            layout()->addWidget(level);
        }
    }

    QList<qreal> levels = getBufferLevels(buffer);
    for (int i = 0; i < levels.count(); ++i)
        m_audioLevels.at(i)->setLevel(levels.at(i));
}

void HistogramWidget::setHistogram(const QList<qreal> &histogram)
{
    m_isBusy = false;
    m_histogram = histogram;
    update();
}

void HistogramWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if (!m_audioLevels.isEmpty())
        return;

    QPainter painter(this);

    if (m_histogram.isEmpty()) {
        painter.fillRect(0, 0, width(), height(), QColor::fromRgb(0, 0, 0));
        return;
    }

    qreal barWidth = width() / (qreal)m_histogram.size();

    for (int i = 0; i < m_histogram.size(); ++i) {
        qreal h = m_histogram[i] * height();
        // draw level
        painter.fillRect(barWidth * i, height() - h, barWidth * (i + 1), height(), Qt::red);
        // clear the rest of the control
        painter.fillRect(barWidth * i, 0, barWidth * (i + 1), height() - h, Qt::black);
    }
}

void FrameProcessor::processFrame(QVideoFrame frame, int levels)
{
    QList<qreal> histogram(levels);

    do {
        if (!levels)
            break;

        if (!frame.map(QAbstractVideoBuffer::ReadOnly))
            break;

        if (frame.pixelFormat() == QVideoFrame::Format_YUV420P ||
            frame.pixelFormat() == QVideoFrame::Format_NV12) {
            // Process YUV data
            uchar *b = frame.bits();
            for (int y = 0; y < frame.height(); ++y) {
                uchar *lastPixel = b + frame.width();
                for (uchar *curPixel = b; curPixel < lastPixel; curPixel++)
                    histogram[(*curPixel * levels) >> 8] += 1.0;
                b += frame.bytesPerLine();
            }
        } else {
            QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
            if (imageFormat != QImage::Format_Invalid) {
                // Process RGB data
                QImage image(frame.bits(), frame.width(), frame.height(), imageFormat);
                image = image.convertToFormat(QImage::Format_RGB32);

                const QRgb* b = (const QRgb*)image.bits();
                for (int y = 0; y < image.height(); ++y) {
                    const QRgb *lastPixel = b + frame.width();
                    for (const QRgb *curPixel = b; curPixel < lastPixel; curPixel++)
                        histogram[(qGray(*curPixel) * levels) >> 8] += 1.0;
                    b = (const QRgb*)((uchar*)b + image.bytesPerLine());
                }
            }
        }

        // find maximum value
        qreal maxValue = 0.0;
        for (double i : qAsConst(histogram)) {
            if (i > maxValue)
                maxValue = i;
        }

        if (maxValue > 0.0) {
            for (double &i : histogram)
                i /= maxValue;
        }

        frame.unmap();
    } while (false);

    emit histogramReady(histogram);
}

#include "histogramwidget.moc"
