// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qopenglvideobuffer_p.h"

#include <qoffscreensurface.h>
#include <qthread.h>
#include <private/qimagevideobuffer_p.h>

#include <QtOpenGL/private/qopenglcompositor_p.h>
#include <QtOpenGL/private/qopenglframebufferobject_p.h>

QT_BEGIN_NAMESPACE

static QOpenGLContext *createContext(QOpenGLContext *shareContext)
{
    // Create an OpenGL context for the current thread. The lifetime of the context is tied to the
    // lifetime of the current thread.
    auto context = std::make_unique<QOpenGLContext>();
    context->setShareContext(shareContext);

    if (!context->create()) {
        qWarning() << "Couldn't create an OpenGL context for QOpenGLVideoBuffer";
        return nullptr;
    }

    QObject::connect(QThread::currentThread(), &QThread::finished,
                     context.get(), &QOpenGLContext::deleteLater);
    return context.release();
}

static bool setCurrentOpenGLContext()
{
    auto compositorContext = QOpenGLCompositor::instance()->context();

    // A thread-local variable is used to avoid creating a new context if we're called on the same
    // thread. The context lifetime is tied to the current thread lifetime (see createContext()).
    static thread_local QOpenGLContext *context = nullptr;
    static thread_local QOffscreenSurface *surface = nullptr;

    if (!context) {
        context = compositorContext->thread()->isCurrentThread() ? compositorContext
                                                                 : createContext(compositorContext);

        if (!context)
            return false;

        surface = new QOffscreenSurface(nullptr, context);
        surface->setFormat(context->format());
        surface->create();
    }

    return context->makeCurrent(surface);
}

QOpenGLVideoBuffer::QOpenGLVideoBuffer(std::unique_ptr<QOpenGLFramebufferObject> fbo)
    : QHwVideoBuffer(QVideoFrame::RhiTextureHandle), m_fbo(std::move(fbo))
{
    Q_ASSERT(m_fbo);
}

QOpenGLVideoBuffer::~QOpenGLVideoBuffer() = default;

QAbstractVideoBuffer::MapData QOpenGLVideoBuffer::map(QVideoFrame::MapMode mode)
{
    return ensureImageBuffer().map(mode);
}

void QOpenGLVideoBuffer::unmap()
{
    if (m_imageBuffer)
        m_imageBuffer->unmap();
}

quint64 QOpenGLVideoBuffer::textureHandle(QRhi &, int plane)
{
    Q_UNUSED(plane);
    return m_fbo->texture();
}

QImageVideoBuffer &QOpenGLVideoBuffer::ensureImageBuffer()
{
    // Create image buffer if not yet created.
    // This is protected by mapMutex in QVideoFrame::map.
    if (!m_imageBuffer) {
        if (!setCurrentOpenGLContext())
            qWarning() << "Failed to set current OpenGL context";

        m_imageBuffer = std::make_unique<QImageVideoBuffer>(m_fbo->toImage(false));
    }

    return *m_imageBuffer;
}

QT_END_NAMESPACE
