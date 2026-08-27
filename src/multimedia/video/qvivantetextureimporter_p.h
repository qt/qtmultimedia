// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QVIVANTETEXTUREIMPORTER_P_H
#define QVIVANTETEXTUREIMPORTER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qhwvideobuffer_p.h"

#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/private/qdmabuftextureimporter_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

#include <QtGui/qopenglfunctions.h>

#include <QtCore/qsize.h>
#include <QtCore/private/qexpected_p.h>

#include <EGL/egl.h>

#include <memory>

static_assert(QT_CONFIG(gpu_vivante));

#ifndef GL_VIV_YV12
#define GL_VIV_YV12 0x8FC0
#endif
#ifndef GL_VIV_NV12
#define GL_VIV_NV12 0x8FC1
#endif
#ifndef GL_VIV_YUY2
#define GL_VIV_YUY2 0x8FC2
#endif
#ifndef GL_VIV_UYVY
#define GL_VIV_UYVY 0x8FC3
#endif
#ifndef GL_VIV_NV21
#define GL_VIV_NV21 0x8FC4
#endif
#ifndef GL_VIV_I420
#define GL_VIV_I420 0x8FC5
#endif

static_assert(GL_VIV_YV12 == 0x8FC0);
static_assert(GL_VIV_NV12 == 0x8FC1);
static_assert(GL_VIV_YUY2 == 0x8FC2);
static_assert(GL_VIV_UYVY == 0x8FC3);
static_assert(GL_VIV_NV21 == 0x8FC4);
static_assert(GL_VIV_I420 == 0x8FC5);

QT_BEGIN_NAMESPACE

class QRhi;
class QOpenGLContext;

namespace QtMultimediaPrivate {

class Q_MULTIMEDIA_EXPORT QVivanteGLFunctions
{
    QVivanteGLFunctions();

public:
    static const QVivanteGLFunctions &instance();

    bool isValid() const;

    void glTexDirectVIVMap(GLenum target, GLsizei width, GLsizei height, GLenum format,
                           GLvoid **logical, const GLuint *physical) const;
    void glTexDirectInvalidateVIV(GLenum target) const;

private:
    typedef void(QOPENGLF_APIENTRYP PFNGLTEXDIRECTVIVMAPPROC)(GLenum Target, GLsizei Width,
                                                              GLsizei Height, GLenum Format,
                                                              GLvoid **Logical,
                                                              const GLuint *Physical);
    typedef void(QOPENGLF_APIENTRYP PFNGLTEXDIRECTINVALIDATEVIVPROC)(GLenum Target);

    PFNGLTEXDIRECTVIVMAPPROC m_glTexDirectVIVMap = nullptr;
    PFNGLTEXDIRECTINVALIDATEVIVPROC m_glTexDirectInvalidateVIV = nullptr;
};

class Q_MULTIMEDIA_EXPORT VivanteContext
{
public:
    explicit VivanteContext(QRhi *);

    bool isValid() const { return m_valid; }

    QOpenGLContext *glContext() const { return m_glContext; }

private:
    QOpenGLContext *m_glContext = nullptr;
    bool m_valid = false;
};

// Owns exactly one GL texture id. Unlike DMA-BUF import (one EGLImage-backed
// texture per plane), glTexDirectVIVMap produces a single combined texture
// per frame: the format token tells the driver how to interpret/convert the
// (possibly planar) memory, and the result is expected to already be
// sampleable as plain RGB, with YUV->RGB conversion done by the driver, not
// a downstream shader. textureHandle() returns that one texture regardless
// of plane index; downstream code should treat this as single-plane RGB.
class Q_MULTIMEDIA_EXPORT VivanteTextureHandles final : public QVideoFrameTexturesHandles
{
public:
    VivanteTextureHandles(QRhi &, QOpenGLContext *glContext, GLuint texture,
                          std::shared_ptr<void> parentKeepAlive = {});

    ~VivanteTextureHandles() override;

    quint64 textureHandle(QRhi &, int /*plane*/) override { return m_texture; }

private:
    void release(QRhi &rhi);

    const std::shared_ptr<void> m_parentKeepAlive; // keep the backend alive
    QRhi& m_rhi;
    QOpenGLContext *const m_glContext;
    const GLuint m_texture;
    bool m_released = false;
};

// physicalAddress is the actual 32-bit hardware address of the mapped
// memory (e.g. from gst_phys_memory_get_phys_addr()), not a host pointer --
// glTexDirectVIVMap's "Physical" parameter takes an array of such
// addresses. logicalAddr must be a valid CPU-virtual mapping of the same
// memory, kept mapped for as long as the texture exists (e.g. via
// parentKeepAlive).
Q_MULTIMEDIA_EXPORT q23::expected<QVideoFrameTexturesHandlesUPtr, FailureSeverity>
importVivanteTexture(QRhi &, const VivanteContext &context, quint32 physicalAddress,
                     void *logicalAddr, QSize, GLenum vivFormatToken,
                     std::shared_ptr<void> parentKeepAlive = {});

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QVIVANTETEXTUREIMPORTER_P_H
