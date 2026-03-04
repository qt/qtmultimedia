// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qthreadlocalrhi_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qthreadstorage.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qpa/qplatformintegration.h>

#if defined(Q_OS_ANDROID)
#  include <QtCore/qmetaobject.h>
#endif

QT_BEGIN_NAMESPACE

namespace {

static thread_local QRhi::Implementation s_preferredBackend = QRhi::Null;

#if QT_CONFIG(opengl)

bool openGLCapsSupported(const QPlatformIntegration &qpa)
{
     return qpa.hasCapability(QPlatformIntegration::OpenGL) &&
            !QCoreApplication::testAttribute(Qt::AA_ForceRasterWidgets) &&
            (QThread::isMainThread() ||
                (qpa.hasCapability(QPlatformIntegration::ThreadedOpenGL) &&
                 qpa.hasCapability(QPlatformIntegration::OffscreenSurface)));
}
#endif

class ThreadLocalRhiHolder
{
public:
    ~ThreadLocalRhiHolder() { resetRhi(); }

    QRhi *ensureRhi([[maybe_unused]] QRhi::Implementation backend)
    {
        if (m_rhi || m_cpuOnly)
            return m_rhi.get();

        const QPlatformIntegration *qpa = QGuiApplicationPrivate::platformIntegration();

        if (qpa && qpa->hasCapability(QPlatformIntegration::RhiBasedRendering)) {

#if QT_CONFIG(metal)
            if (canUseRhiImpl(QRhi::Metal, backend)) {
                QRhiMetalInitParams params;
                m_rhi.reset(QRhi::create(QRhi::Metal, &params));
            }
#endif

#if defined(Q_OS_WIN)
            if (!m_rhi && canUseRhiImpl(QRhi::D3D11, backend)) {
                QRhiD3D11InitParams params;
                m_rhi.reset(QRhi::create(QRhi::D3D11, &params));
            }
#endif

#if QT_CONFIG(opengl)
            if (!m_rhi && canUseRhiImpl(QRhi::OpenGLES2, backend)) {
                if (openGLCapsSupported(*qpa)) {

                    m_fallbackSurface.reset(QRhiGles2InitParams::newFallbackSurface());
                    QRhiGles2InitParams params;
                    params.fallbackSurface = m_fallbackSurface.get();
                    m_rhi.reset(QRhi::create(QRhi::OpenGLES2, &params));

#  if defined(Q_OS_ANDROID)
                    // reset RHI state on application suspension, as this will be invalid after
                    // resuming
                    if (!m_appStateChangedConnection) {
                        if (!m_eventsReceiver)
                            m_eventsReceiver = std::make_unique<QObject>();

                        auto onStateChanged = [this](auto state) {
                            if (state == Qt::ApplicationSuspended)
                                resetRhi();
                        };

                        m_appStateChangedConnection =
                                QObject::connect(qApp, &QGuiApplication::applicationStateChanged,
                                                 m_eventsReceiver.get(), onStateChanged);
                    }
#  endif
                }
            }
#endif
        }

        if (!m_rhi) {
            m_cpuOnly = true;
            qWarning() << Q_FUNC_INFO << ": No RHI backend. Using CPU conversion.";
        }

        return m_rhi.get();
    }

    void resetRhi()
    {
        m_rhi.reset();
#if QT_CONFIG(opengl)
        m_fallbackSurface.reset();
#endif
        m_cpuOnly = false;
    }

    bool canUseRhiImpl(const QRhi::Implementation implementation,
                       const QRhi::Implementation reference)
    {
        // First priority goes to reference backend
        if (reference != QRhi::Null)
            return implementation == reference;

        // If no reference, but preference exists, compare to that
        if (s_preferredBackend != QRhi::Null)
            return implementation == s_preferredBackend;

        // Can use (assuming platform and configuration allow)
        return true;
    }

private:
    std::unique_ptr<QRhi> m_rhi;
#if QT_CONFIG(opengl)
    std::unique_ptr<QOffscreenSurface> m_fallbackSurface;
#endif
    bool m_cpuOnly = false;
#if defined(Q_OS_ANDROID)
    std::unique_ptr<QObject> m_eventsReceiver;
    // we keep and check QMetaObject::Connection because the sender, qApp,
    // can be recreated and the connection invalidated.
    QMetaObject::Connection m_appStateChangedConnection;
#endif
};

QThreadStorage<ThreadLocalRhiHolder> g_threadLocalRhiHolder;

}

QRhi *qEnsureThreadLocalRhi(QRhi::Implementation backend)
{
    return g_threadLocalRhiHolder.localData().ensureRhi(backend);
}

void qSetPreferredThreadLocalRhiBackend(QRhi::Implementation backend)
{
    if (s_preferredBackend != backend) {
        s_preferredBackend = backend;
        g_threadLocalRhiHolder.localData().resetRhi();
    }
}

QT_END_NAMESPACE
