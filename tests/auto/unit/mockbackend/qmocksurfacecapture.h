// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QMOCKSURFACECAPTURE_H
#define QMOCKSURFACECAPTURE_H

#include "private/qplatformsurfacecapture_p.h"
#include "private/qvideoframe_p.h"

#include "qmockvideobuffer.h"
#include "qthread.h"

QT_BEGIN_NAMESPACE

class QMockSurfaceCapture : public QPlatformSurfaceCapture
{
    class Grabber : public QThread
    {
    public:
        Grabber(QMockSurfaceCapture &capture) : QThread(&capture), m_capture(capture) { }

        void run() override
        {
            for (int i = 0; !isInterruptionRequested(); ++i) {
                QImage image(m_capture.m_imageSize, QImage::Format_ARGB32);

                image.fill(i % 2 ? Qt::red : Qt::blue);

                QVideoFrame frame = QVideoFramePrivate::createFrame(
                        std::make_unique<QMockVideoBuffer>(image),
                        QVideoFrameFormat(m_capture.m_imageSize,
                                          QVideoFrameFormat::pixelFormatFromImageFormat(
                                                  m_capture.m_imageFormat)));

                emit m_capture.newVideoFrame(frame);
            }
        }

    private:
        QMockSurfaceCapture &m_capture;
    };

public:
    using QPlatformSurfaceCapture::QPlatformSurfaceCapture;

    ~QMockSurfaceCapture() { resetGrabber(); }

    bool setActiveInternal(bool active) override
    {
        if (active) {
            m_grabber = std::make_unique<Grabber>(*this);
            m_grabber->start();
        } else {
            resetGrabber();
        }

        return true;
    }

    bool isActive() const override { return bool(m_grabber); }

    QVideoFrameFormat frameFormat() const override
    {
        return m_grabber ? QVideoFrameFormat(
                       m_imageSize, QVideoFrameFormat::pixelFormatFromImageFormat(m_imageFormat))
                         : QVideoFrameFormat{};
    }

private:
    void resetGrabber()
    {
        if (m_grabber) {
            m_grabber->requestInterruption();
            m_grabber->quit();
            m_grabber->wait();
            m_grabber.reset();
        }
    }

private:
    std::unique_ptr<Grabber> m_grabber;
    const QImage::Format m_imageFormat = QImage::Format_ARGB32;
    const QSize m_imageSize = QSize(2, 3);
};

QT_END_NAMESPACE

#endif // QMOCKSURFACECAPTURE_H
