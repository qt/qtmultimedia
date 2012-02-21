/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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

HistogramWidget::HistogramWidget(QWidget *parent)
    : QWidget(parent)
    , m_levels(128)
{
}

void HistogramWidget::processFrame(QVideoFrame frame)
{
    m_histogram.clear();

    do {
        if (!m_levels)
            break;

        if (!frame.map(QAbstractVideoBuffer::ReadOnly))
            break;

        m_histogram.resize(m_levels);

        if (frame.pixelFormat() == QVideoFrame::Format_YUV420P) {
            // Process YUV420P data
            uchar *b = frame.bits();
            for (int y = 0; y < frame.height(); y++) {
                uchar *lastPixel = b + frame.width();
                for (uchar *curPixel = b; curPixel < lastPixel; curPixel++)
                    m_histogram[(*curPixel * m_levels) / 256] += 1.0;
                b += frame.bytesPerLine();
            }
        } else {
            QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
            if (imageFormat != QImage::Format_Invalid) {
                // Process RGB data
                QImage image(frame.bits(), frame.width(), frame.height(), imageFormat);
                image = image.convertToFormat(QImage::Format_RGB32);

                const QRgb* b = (const QRgb*)image.bits();
                for (int y = 0; y < image.height(); y++) {
                    const QRgb *lastPixel = b + frame.width();
                    for (const QRgb *curPixel = b; curPixel < lastPixel; curPixel++)
                        m_histogram[(qGray(*curPixel) * m_levels) / 256] += 1.0;
                    b = (const QRgb*)((uchar*)b + image.bytesPerLine());
                }
            }
        }

        // find maximum value
        qreal maxValue = 0.0;
        for (int i = 0; i < m_histogram.size(); i++) {
            if (m_histogram[i] > maxValue)
                maxValue = m_histogram[i];
        }

        if (maxValue > 0.0) {
            for (int i = 0; i < m_histogram.size(); i++)
                m_histogram[i] /= maxValue;
        }

        frame.unmap();
    } while (false);

    update();
}

void HistogramWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    if (m_histogram.isEmpty()) {
        painter.fillRect(0, 0, width(), height(), QColor::fromRgb(0, 0, 0));
        return;
    }

    qreal barWidth = width() / (qreal)m_histogram.size();

    for (int i = 0; i < m_histogram.size(); i++) {
        qreal h = m_histogram[i] * height();
        // draw level
        painter.fillRect(barWidth * i, height() - h, barWidth * (i + 1), height(), Qt::red);
        // clear the rest of the control
        painter.fillRect(barWidth * i, 0, barWidth * (i + 1), height() - h, Qt::black);
    }
}
