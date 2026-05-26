// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSURFACEIMAGE_P_H
#define QOHOSSURFACEIMAGE_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qmutex.h>
#include <QtGui/qmatrix4x4.h>

#include <native_image/native_image.h>

#include <atomic>

QT_BEGIN_NAMESPACE

class QOhosSurfaceImage : public QObject
{
    Q_OBJECT
public:
    explicit QOhosSurfaceImage(uint32_t glTextureId, QObject *parent = nullptr);
    ~QOhosSurfaceImage() override;

    Q_DISABLE_COPY_MOVE(QOhosSurfaceImage)

    bool isValid() const { return m_image != nullptr; }

    OHNativeWindow *nativeWindow() const { return m_nativeWindow; }
    QByteArray surfaceId() const { return m_surfaceId; }

    // Must be called on the GL context thread that owns m_textureId.
    bool updateTexImage();
    QMatrix4x4 transformMatrix() const;

    quint64 index() const { return m_index; }

signals:
    void frameAvailable(quint64 index);

private:
    static void onFrameAvailableTrampoline(void *context);

    OH_NativeImage *m_image{ nullptr };
    OHNativeWindow *m_nativeWindow{ nullptr };
    uint32_t m_textureId{ 0 };
    QByteArray m_surfaceId;
    quint64 m_index{ 0 };
    static std::atomic<quint64> s_nextIndex;
};

QT_END_NAMESPACE

#endif // QOHOSSURFACEIMAGE_P_H
