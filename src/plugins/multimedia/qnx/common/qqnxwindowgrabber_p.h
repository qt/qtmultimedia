/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
};

QT_END_NAMESPACE

#endif
