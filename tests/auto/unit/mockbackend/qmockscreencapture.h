// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMOCKSCREENCAPTURE_H
#define QMOCKSCREENCAPTURE_H

#include "private/qplatformscreencapture_p.h"

#include "qmockvideobuffer.h"
#include "qthread.h"

QT_BEGIN_NAMESPACE

class QMockScreenCapture : public QPlatformScreenCapture
{
    class Grabber : public QThread
    {
    public:
        Grabber(QMockScreenCapture &capture) : QThread(&capture), m_capture(capture) { }

        void run() override
        {
            for (int i = 0; !isInterruptionRequested(); ++i) {
                QImage image(m_capture.m_imageSize, QImage::Format_ARGB32);

                image.fill(i % 2 ? Qt::red : Qt::blue);

                QVideoFrame frame(new QMockVideoBuffer(image),
                                  QVideoFrameFormat(m_capture.m_imageSize,
                                                    QVideoFrameFormat::pixelFormatFromImageFormat(
                                                            m_capture.m_imageFormat)));

                emit m_capture.newVideoFrame(frame);
            }
        }

    private:
        QMockScreenCapture &m_capture;
    };

public:
    using QPlatformScreenCapture::QPlatformScreenCapture;

    ~QMockScreenCapture() { resetGrabber(); }

    void setActive(bool active) override
    {
        if (active == bool(m_grabber))
            return;

        if (m_grabber) {
            resetGrabber();
        } else {
            m_grabber = std::make_unique<Grabber>(*this);
            m_grabber->start();
        }

        emit screenCapture()->activeChanged(bool(m_grabber));
    }

    bool isActive() const override { return bool(m_grabber); }

    QVideoFrameFormat format() const override
    {
        return m_grabber ? QVideoFrameFormat(
                       m_imageSize, QVideoFrameFormat::pixelFormatFromImageFormat(m_imageFormat))
                         : QVideoFrameFormat{};
    }

    void setScreen(QScreen *) override { }

    QScreen *screen() const override { return nullptr; }

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

#endif // QMOCKSCREENCAPTURE_H
