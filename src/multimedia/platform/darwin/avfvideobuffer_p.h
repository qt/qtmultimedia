/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef AVFVIDEOBUFFER_H
#define AVFVIDEOBUFFER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/qvideoframe.h>
#include <private/qabstractvideobuffer_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qmutex.h>
#include <private/avfvideosink_p.h>

#include <CoreVideo/CVBase.h>
#include <CoreVideo/CVPixelBuffer.h>
#include <CoreVideo/CVImageBuffer.h>

#import "Metal/Metal.h"
#import "MetalKit/MetalKit.h"

enum {
    // macOS 10.14 doesn't define this pixel format yet
    q_kCVPixelFormatType_OneComponent16 = 'L016'
};

QT_BEGIN_NAMESPACE

struct AVFMetalTexture;
class AVFVideoBuffer : public QAbstractVideoBuffer
{
public:
    AVFVideoBuffer(AVFVideoSinkInterface *sink, CVImageBufferRef buffer);
    ~AVFVideoBuffer();

    QVideoFrameFormat::PixelFormat fromCVVideoPixelFormat(unsigned avPixelFormat) const;

    static QVideoFrameFormat::PixelFormat fromCVPixelFormat(unsigned avPixelFormat);
    static bool toCVPixelFormat(QVideoFrameFormat::PixelFormat qtFormat, unsigned &conv);


    QVideoFrame::MapMode mapMode() const { return m_mode; }
    MapData map(QVideoFrame::MapMode mode);
    void unmap();

    virtual quint64 textureHandle(int plane) const;

private:
    AVFVideoSinkInterface *sink = nullptr;

    mutable CVMetalTextureRef cvMetalTexture[3] = {};

#if defined(Q_OS_MACOS)
    mutable CVOpenGLTextureRef cvOpenGLTexture = nullptr;
#elif defined(Q_OS_IOS)
    mutable CVOpenGLESTextureRef cvOpenGLESTexture = nullptr;
#endif

    CVImageBufferRef m_buffer = nullptr;
    QVideoFrame::MapMode m_mode = QVideoFrame::NotMapped;
    QVideoFrameFormat::PixelFormat m_pixelFormat = QVideoFrameFormat::Format_Invalid;
};

QT_END_NAMESPACE

#endif
