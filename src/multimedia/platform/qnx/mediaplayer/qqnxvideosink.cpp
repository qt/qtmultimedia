/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
** Copyright (C) 2021 The Qt Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqnxvideosink_p.h"

#include "qqnxwindowgrabber_p.h"

#include <QCoreApplication>
#include <QDebug>
#include <QVideoFrameFormat>
#include <qvideosink.h>
#include <qvideoframe.h>
#include <private/qabstractvideobuffer_p.h>
#include <QOpenGLContext>

#include <mm/renderer.h>

QT_BEGIN_NAMESPACE

static int winIdCounter = 0;

QQnxVideoSink::QQnxVideoSink(QVideoSink *parent)
    : QPlatformVideoSink(parent)
    , m_QQnxWindowGrabber(new QQnxWindowGrabber(this))
    , m_context(0)
    , m_videoId(-1)
{
    connect(m_QQnxWindowGrabber, SIGNAL(updateScene(const QSize &)), SLOT(updateScene(const QSize &)));

    // ### probably needs to be done on setRhi()!
    if (QOpenGLContext::currentContext())
        m_QQnxWindowGrabber->checkForEglImageExtension();
    // ####
    //    else if (sink)
    //        sink->setProperty("_q_GLThreadCallback", QVariant::fromValue<QObject*>(this));
}

QQnxVideoSink::~QQnxVideoSink()
{
    detachDisplay();
}

void QQnxVideoSink::attachDisplay(mmr_context_t *context)
{
    if (m_videoId != -1) {
        qWarning() << "QQnxVideoSink: Video output already attached!";
        return;
    }

    if (!context) {
        qWarning() << "QQnxVideoSink: No media player context!";
        return;
    }

    const QByteArray windowGroupId = m_QQnxWindowGrabber->windowGroupId();
    if (windowGroupId.isEmpty()) {
        qWarning() << "QQnxVideoSink: Unable to find window group";
        return;
    }

    const QString windowName = QStringLiteral("QQnxVideoSink_%1_%2")
                                             .arg(winIdCounter++)
                                             .arg(QCoreApplication::applicationPid());

    m_QQnxWindowGrabber->setWindowId(windowName.toLatin1());

    // Start with an invisible window, because we just want to grab the frames from it.
    const QString videoDeviceUrl = QStringLiteral("screen:?winid=%1&wingrp=%2&initflags=invisible&nodstviewport=1")
                                                 .arg(windowName)
                                                 .arg(QString::fromLatin1(windowGroupId));

    m_videoId = mmr_output_attach(context, videoDeviceUrl.toLatin1(), "video");
    if (m_videoId == -1) {
        qWarning() << "mmr_output_attach() for video failed";
        return;
    }

    m_context = context;
}

void QQnxVideoSink::detachDisplay()
{
    m_QQnxWindowGrabber->stop();

    if (sink)
        sink->setVideoFrame({});

    if (m_context && m_videoId != -1)
        mmr_output_detach(m_context, m_videoId);

    m_context = 0;
    m_videoId = -1;
}

void QQnxVideoSink::pause()
{
    m_QQnxWindowGrabber->pause();
}

void QQnxVideoSink::resume()
{
    m_QQnxWindowGrabber->resume();
}

class QnxTextureBuffer : public QAbstractVideoBuffer
{
public:
    QnxTextureBuffer(QQnxWindowGrabber *QQnxWindowGrabber)
        : QAbstractVideoBuffer(QVideoFrame::RhiTextureHandle)
    {
        m_QQnxWindowGrabber = QQnxWindowGrabber;
        m_handle = 0;
    }
    QVideoFrame::MapMode mapMode() const override {
        return QVideoFrame::ReadWrite;
    }
    void unmap() override {}
    MapData map(QVideoFrame::MapMode /*mode*/) override { return {}; }
    quint64 textureHandle(int plane) const override {
        if (plane != 0)
            return 0;
        if (!m_handle) {
            const_cast<QnxTextureBuffer*>(this)->m_handle = m_QQnxWindowGrabber->getNextTextureId();
        }
        return m_handle;
    }
private:
    QQnxWindowGrabber *m_QQnxWindowGrabber;
    quint64 m_handle;
};

void QQnxVideoSink::updateScene(const QSize &size)
{
    if (sink) {
        // Depending on the support of EGL images on the current platform we either pass a texture
        // handle or a copy of the image data
        if (m_QQnxWindowGrabber->eglImageSupported()) {
            QnxTextureBuffer *textBuffer = new QnxTextureBuffer(m_QQnxWindowGrabber);
            QVideoFrame actualFrame(textBuffer, QVideoFrameFormat(size, QVideoFrameFormat::Format_BGRX8888));
            sink->setVideoFrame(actualFrame);
            // ####
//        } else {
//            sink->setVideoFrame(m_QQnxWindowGrabber->getNextImage().copy());
        }
    }
}

void QQnxVideoSink::customEvent(QEvent *e)
{
    // This is running in the render thread (OpenGL enabled)
    if (e->type() == QEvent::User)
        m_QQnxWindowGrabber->checkForEglImageExtension();
}

QT_END_NAMESPACE
