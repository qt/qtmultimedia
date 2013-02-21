/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <AppKit/AppKit.h>
#include <QuartzCore/CIContext.h>
#include <CGLCurrent.h>
#include <OpenGL/gl.h>

#include "qpaintervideosurface_mac_p.h"

#include <QtCore/qdatetime.h>

#include <qmath.h>

#include <qpainter.h>
#include <qvariant.h>
#include <qvideosurfaceformat.h>

#include <QtDebug>

QT_BEGIN_NAMESPACE

extern CGContextRef qt_mac_cg_context(const QPaintDevice *pdev); //qpaintdevice_mac.cpp

QVideoSurfaceCoreGraphicsPainter::QVideoSurfaceCoreGraphicsPainter(bool glSupported)
    : ciContext(0)
    , m_imageFormat(QImage::Format_Invalid)
    , m_scanLineDirection(QVideoSurfaceFormat::TopToBottom)
{
    //qDebug() << "QVideoSurfaceCoreGraphicsPainter, GL supported:" << glSupported;
    ciContext = 0;
    m_imagePixelFormats
        << QVideoFrame::Format_RGB32
        << QVideoFrame::Format_ARGB32
        << QVideoFrame::Format_ARGB32_Premultiplied
        << QVideoFrame::Format_BGR32
        << QVideoFrame::Format_RGB24
        << QVideoFrame::Format_RGB565
        << QVideoFrame::Format_RGB555
        << QVideoFrame::Format_ARGB8565_Premultiplied;

    m_supportedHandles
            << QAbstractVideoBuffer::NoHandle
            << QAbstractVideoBuffer::CoreImageHandle;

    if (glSupported)
            m_supportedHandles << QAbstractVideoBuffer::GLTextureHandle;
}

QVideoSurfaceCoreGraphicsPainter::~QVideoSurfaceCoreGraphicsPainter()
{
    [(CIContext*)ciContext release];
}

QList<QVideoFrame::PixelFormat> QVideoSurfaceCoreGraphicsPainter::supportedPixelFormats(
        QAbstractVideoBuffer::HandleType handleType) const
{
    return m_supportedHandles.contains(handleType)
        ? m_imagePixelFormats
        : QList<QVideoFrame::PixelFormat>();
}

bool QVideoSurfaceCoreGraphicsPainter::isFormatSupported(const QVideoSurfaceFormat &format) const
{
    return m_supportedHandles.contains(format.handleType())
            && m_imagePixelFormats.contains(format.pixelFormat())
            && !format.frameSize().isEmpty();
}

QAbstractVideoSurface::Error QVideoSurfaceCoreGraphicsPainter::start(const QVideoSurfaceFormat &format)
{
    m_frame = QVideoFrame();
    m_imageFormat = QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());
    m_imageSize = format.frameSize();
    m_scanLineDirection = format.scanLineDirection();

    return m_supportedHandles.contains(format.handleType())
            && ((m_imageFormat != QImage::Format_Invalid) || (format.handleType() == QAbstractVideoBuffer::GLTextureHandle))
            && !m_imageSize.isEmpty()
            ? QAbstractVideoSurface::NoError
            : QAbstractVideoSurface::UnsupportedFormatError;
}

void QVideoSurfaceCoreGraphicsPainter::stop()
{
    m_frame = QVideoFrame();
}

QAbstractVideoSurface::Error QVideoSurfaceCoreGraphicsPainter::setCurrentFrame(const QVideoFrame &frame)
{
    m_frame = frame;

    return QAbstractVideoSurface::NoError;
}

QAbstractVideoSurface::Error QVideoSurfaceCoreGraphicsPainter::paint(
            const QRectF &target, QPainter *painter, const QRectF &source)
{
    if (m_frame.handleType() == QAbstractVideoBuffer::CoreImageHandle) {
//Non OpenGL CI rendering is disabled for now since qt_mac_cg_context is moved to platform plugin
#ifdef ENABLE_CORE_GRAPHICS_VIDEO_RENDERING
        if (painter->paintEngine()->type() == QPaintEngine::CoreGraphics ) {

            CIImage *img = (CIImage*)(m_frame.handle().value<void*>());

            if (img) {
                CGContextRef cgContext = qt_mac_cg_context(painter->device());

                if (cgContext) {
                    painter->beginNativePainting();

                    CGRect sRect = CGRectMake(source.x(), source.y(), source.width(), source.height());
                    CGRect dRect = CGRectMake(target.x(), target.y(), target.width(), target.height());

                    NSBitmapImageRep *bitmap = [[NSBitmapImageRep alloc] initWithCIImage:img];

                    if (m_scanLineDirection == QVideoSurfaceFormat::TopToBottom) {
                        CGContextSaveGState( cgContext );
                        CGContextTranslateCTM(cgContext, 0, dRect.origin.y + CGRectGetMaxY(dRect));
                        CGContextScaleCTM(cgContext, 1, -1);

                        CGContextDrawImage(cgContext, dRect, [bitmap CGImage]);

                        CGContextRestoreGState(cgContext);
                    } else {
                        CGContextDrawImage(cgContext, dRect, [bitmap CGImage]);
                    }

                    [bitmap release];

                    painter->endNativePainting();

                    return QAbstractVideoSurface::NoError;
                }
            }
        } else
#endif
        if (painter->paintEngine()->type() == QPaintEngine::OpenGL2 ||
                   painter->paintEngine()->type() == QPaintEngine::OpenGL) {
            CIImage *img = (CIImage*)(m_frame.handle().value<void*>());

            if (img) {
                CGLContextObj cglContext = CGLGetCurrentContext();

                if (cglContext) {

                    if (!ciContext) {
                        CGLContextObj cglContext = CGLGetCurrentContext();
                        NSOpenGLPixelFormat *nsglPixelFormat = [NSOpenGLView defaultPixelFormat];
                        CGLPixelFormatObj cglPixelFormat = static_cast<CGLPixelFormatObj>([nsglPixelFormat CGLPixelFormatObj]);

                        ciContext = [CIContext contextWithCGLContext:cglContext
                                     pixelFormat:cglPixelFormat
                                         options:nil];

                        [(CIContext*)ciContext retain];
                    }

                    CGRect sRect = CGRectMake(source.x(), source.y(), source.width(), source.height());
                    CGRect dRect = m_scanLineDirection == QVideoSurfaceFormat::TopToBottom ?
                                   CGRectMake(target.x(), target.y()+target.height(), target.width(), -target.height()) :
                                   CGRectMake(target.x(), target.y(), target.width(), target.height());


                    painter->beginNativePainting();

                    [(CIContext*)ciContext drawImage:img inRect:dRect fromRect:sRect];

                    painter->endNativePainting();

                    return QAbstractVideoSurface::NoError;
                }
            }
        }
    }

    if (m_frame.handleType() == QAbstractVideoBuffer::GLTextureHandle &&
               (painter->paintEngine()->type() == QPaintEngine::OpenGL2 ||
                painter->paintEngine()->type() == QPaintEngine::OpenGL)) {

        painter->beginNativePainting();
            GLuint texture = m_frame.handle().toUInt();

            glDisable(GL_CULL_FACE);
            glEnable(GL_TEXTURE_2D);

            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            const float txLeft = source.left() / m_frame.width();
            const float txRight = source.right() / m_frame.width();
            const float txTop = m_scanLineDirection == QVideoSurfaceFormat::TopToBottom
                    ? source.top() / m_frame.height()
                    : source.bottom() / m_frame.height();
            const float txBottom = m_scanLineDirection == QVideoSurfaceFormat::TopToBottom
                    ? source.bottom() / m_frame.height()
                    : source.top() / m_frame.height();

            glBegin(GL_QUADS);
                QRectF rect = target;
                glTexCoord2f(txLeft, txBottom);
                glVertex2f(rect.topLeft().x(), rect.topLeft().y());
                glTexCoord2f(txRight, txBottom);
                glVertex2f(rect.topRight().x() + 1, rect.topRight().y());
                glTexCoord2f(txRight, txTop);
                glVertex2f(rect.bottomRight().x() + 1, rect.bottomRight().y() + 1);
                glTexCoord2f(txLeft, txTop);
                glVertex2f(rect.bottomLeft().x(), rect.bottomLeft().y() + 1);
            glEnd();
        painter->endNativePainting();

        return QAbstractVideoSurface::NoError;
    }

    //fallback case, software rendering
    if (m_frame.map(QAbstractVideoBuffer::ReadOnly)) {
        QImage image(
                m_frame.bits(),
                m_imageSize.width(),
                m_imageSize.height(),
                m_frame.bytesPerLine(),
                m_imageFormat);

        if (m_scanLineDirection == QVideoSurfaceFormat::BottomToTop) {
            const QTransform oldTransform = painter->transform();

            painter->scale(1, -1);
            painter->translate(0, -target.bottom());
            painter->drawImage(
                QRectF(target.x(), 0, target.width(), target.height()), image, source);
            painter->setTransform(oldTransform);
        } else {
            painter->drawImage(target, image, source);
        }

        m_frame.unmap();
    } else if (m_frame.isValid()) {
        return QAbstractVideoSurface::IncorrectFormatError;
    } else {
        painter->fillRect(target, Qt::black);
    }

    return QAbstractVideoSurface::NoError;
}

void QVideoSurfaceCoreGraphicsPainter::updateColors(int, int, int, int)
{
}

QT_END_NAMESPACE
