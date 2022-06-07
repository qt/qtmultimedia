// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQnxWindowGrabber_H
#define QQnxWindowGrabber_H

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

#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/eglext.h>
#include <QAbstractNativeEventFilter>
#include <QObject>
#include <QSize>
#include <QTimer>

#include <memory>

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

class QRhi;
class QQnxWindowGrabberImage;

class QQnxWindowGrabber : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    struct BufferView
    {
        int width = -1;
        int height = -1;
        int stride = -1;

        unsigned char *data = nullptr;

        static constexpr int pixelSize = 4; // BGRX8888;
    };

    explicit QQnxWindowGrabber(QObject *parent = 0);
    ~QQnxWindowGrabber();

    void setFrameRate(int frameRate);

    void setWindowId(const QByteArray &windowId);

    void setRhi(QRhi *rhi);

    void start();
    void stop();

    void pause();
    void resume();

    void forceUpdate();

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

    bool handleScreenEvent(screen_event_t event);

    QByteArray windowGroupId() const;

    bool isEglImageSupported() const;

    int getNextTextureId();
    BufferView getNextBuffer();

signals:
    void updateScene(const QSize &size);

private slots:
    void triggerUpdate();

private:
    bool selectBuffer();
    void resetBuffers();
    void checkForEglImageExtension();

    QTimer m_timer;

    QByteArray m_windowId;

    screen_window_t m_windowParent;
    screen_window_t m_window;
    screen_context_t m_screenContext;

    std::unique_ptr<QQnxWindowGrabberImage> m_frontBuffer;
    std::unique_ptr<QQnxWindowGrabberImage> m_backBuffer;

    QSize m_size;

    QRhi *m_rhi;

    bool m_active;
    bool m_eglImageSupported;
    bool m_startPending;
};

QT_END_NAMESPACE

#endif
