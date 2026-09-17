// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QDMABUFTEXTUREIMPORTER_P_H
#define QDMABUFTEXTUREIMPORTER_P_H

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
#include <QtMultimedia/private/qmultimedia_drm_support_p.h>
#include <QtMultimedia/private/qmultimedia_gl_support_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

#include <QtGui/qopenglfunctions.h>

#include <QtCore/qsize.h>
#include <QtCore/private/qexpected_p.h>

#include <EGL/egl.h>

#include <array>
#include <memory>

static_assert(QT_CONFIG(linux_dmabuf));

QT_BEGIN_NAMESPACE

class QRhi;
class QOpenGLContext;

namespace QtMultimediaPrivate {

class Q_MULTIMEDIA_EXPORT DmaBufEglContext
{
public:
    explicit DmaBufEglContext(QRhi *rhi);

    bool isValid() const { return m_valid; }
    void invalidate() { m_valid = false; }

    QOpenGLContext *glContext() const { return m_glContext; }
    EGLDisplay eglDisplay() const { return m_eglDisplay; }

private:
    QOpenGLContext *m_glContext = nullptr;
    EGLDisplay m_eglDisplay = nullptr;
    bool m_valid = false;
};

class Q_MULTIMEDIA_EXPORT DmaBufTextureHandles final : public QVideoFrameTexturesHandles
{
public:
    DmaBufTextureHandles(QRhi &rhi, std::array<GlTextureHandle, 4> textures,
                         std::shared_ptr<void> parentKeepAlive = {});

    ~DmaBufTextureHandles() override;

    quint64 textureHandle(QRhi &, int plane) override { return m_textures[plane].get(); }

private:
    const std::shared_ptr<void> m_parentKeepAlive; // keep the backend alive
    QRhi &m_rhi;
    const std::array<GlTextureHandle, 4> m_textures;
};

// Severity of a failure to import DMABUF planes as GL textures.
enum class FailureSeverity {
    recoverable, // the caller may retry, e.g. with a different frame
    unrecoverable, // the caller should permanently disable DMABUF import
};

Q_MULTIMEDIA_EXPORT q23::expected<QVideoFrameTexturesHandlesUPtr, FailureSeverity>
importDmaBufTextures(QRhi &rhi, const DmaBufEglContext &eglContext, QSpan<const DmaBufPlane> planes,
                     QVideoFrameFormat::PixelFormat qtFormat, QSize frameSize,
                     std::shared_ptr<void> parentKeepAlive = {});

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QDMABUFTEXTUREIMPORTER_P_H
