// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohossurfaceimage_p.h"

#include "qohosglobal_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qloggingcategory.h>

#include <native_window/external_window.h>

QT_BEGIN_NAMESPACE

namespace {
constexpr uint32_t kGlTextureExternalOes = 0x8D65; // GL_OES_EGL_image_external
} // namespace

std::atomic<quint64> QOhosSurfaceImage::s_nextIndex{ 1 };

QOhosSurfaceImage::QOhosSurfaceImage(uint32_t glTextureId, QObject *parent)
    : QObject(parent), m_textureId(glTextureId)
{
    m_index = s_nextIndex.fetch_add(1);

    m_image = OH_NativeImage_Create(glTextureId, kGlTextureExternalOes);
    if (!m_image) {
        qCWarning(qLcOhosMediaPlugin) << "OH_NativeImage_Create failed for texture" << glTextureId;
        return;
    }

    m_nativeWindow = OH_NativeImage_AcquireNativeWindow(m_image);
    if (!m_nativeWindow) {
        qCWarning(qLcOhosMediaPlugin) << "OH_NativeImage_AcquireNativeWindow returned null";
        OH_NativeImage_Destroy(&m_image);
        return;
    }

    uint64_t surfaceIdRaw = 0;
    if (OH_NativeImage_GetSurfaceId(m_image, &surfaceIdRaw) == 0)
        m_surfaceId = QByteArray::number(qulonglong(surfaceIdRaw));

    OH_OnFrameAvailableListener listener{};
    listener.context = this;
    listener.onFrameAvailable = &QOhosSurfaceImage::onFrameAvailableTrampoline;
    OH_NativeImage_SetOnFrameAvailableListener(m_image, listener);
}

QOhosSurfaceImage::~QOhosSurfaceImage()
{
    if (m_image) {
        OH_NativeImage_UnsetOnFrameAvailableListener(m_image);
        OH_NativeImage_Destroy(&m_image);
    }
}

bool QOhosSurfaceImage::updateTexImage()
{
    if (!m_image)
        return false;
    return OH_NativeImage_UpdateSurfaceImage(m_image) == 0;
}

QMatrix4x4 QOhosSurfaceImage::transformMatrix() const
{
    QMatrix4x4 m;
    if (!m_image)
        return m;
    // OH_NativeImage returns column-major; populate QMatrix4x4's storage directly.
    if (OH_NativeImage_GetTransformMatrixV2(m_image, m.data()) != 0)
        m.setToIdentity();
    return m;
}

void QOhosSurfaceImage::onFrameAvailableTrampoline(void *context)
{
    auto *self = reinterpret_cast<QOhosSurfaceImage *>(context);
    if (!self)
        return;
    // Native producer thread — emit the signal so the GL context thread can
    // pick it up via a queued connection.
    emit self->frameAvailable(self->m_index);
}

QT_END_NAMESPACE

#include "moc_qohossurfaceimage_p.cpp"
