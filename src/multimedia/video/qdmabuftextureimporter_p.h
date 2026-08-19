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
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

#include <QtGui/qopenglfunctions.h>

#include <QtCore/qsize.h>
#include <QtCore/private/qexpected_p.h>

#include <array>
#include <memory>

static_assert(QT_CONFIG(linux_dmabuf));

QT_BEGIN_NAMESPACE

class QRhi;
class QOpenGLContext;

namespace QtMultimediaPrivate {

struct DmaBufPlane
{
    int fd = -1;
    uint32_t offset = 0;
    uint32_t pitch = 0;
    DRMFormat drmFormat = DRMFormat::RGBA8888;
    uint64_t modifier = DmaBufFormatModifierInvalid;
};

class Q_MULTIMEDIA_EXPORT DmaBufEglContext
{
public:
    explicit DmaBufEglContext(QRhi *rhi);

    bool isValid() const { return m_valid; }
    void invalidate() { m_valid = false; }

    QOpenGLContext *glContext() const { return m_glContext; }
    Qt::HANDLE eglDisplay() const { return m_eglDisplay; }
    QFunctionPointer eglImageTargetTexture2D() const { return m_eglImageTargetTexture2D; }

private:
    QOpenGLContext *m_glContext = nullptr;
    Qt::HANDLE m_eglDisplay = nullptr;
    QFunctionPointer m_eglImageTargetTexture2D = nullptr;
    bool m_valid = false;
};

class Q_MULTIMEDIA_EXPORT DmaBufTextureHandles final : public QVideoFrameTexturesHandles
{
public:
    DmaBufTextureHandles(QRhi &rhi, QOpenGLContext *glContext, int nPlanes,
                         std::array<GLuint, 4> textures,
                         std::shared_ptr<void> parentKeepAlive = {});

    ~DmaBufTextureHandles() override;

    quint64 textureHandle(QRhi &, int plane) override { return m_textures[plane]; }

private:
    const std::shared_ptr<void> m_parentKeepAlive; // keep the backend alive
    QRhi &m_rhi;
    QOpenGLContext *const m_glContext;
    const int m_nPlanes;
    const std::array<GLuint, 4> m_textures;
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
