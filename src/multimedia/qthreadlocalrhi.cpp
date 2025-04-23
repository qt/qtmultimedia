// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qthreadlocalrhi_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qthreadstorage.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/rhi/qrhi.h>

#if defined(Q_OS_ANDROID)
#  include <QtCore/qmetaobject.h>
#endif

QT_BEGIN_NAMESPACE

namespace {

class ThreadLocalRhiHolder
{
public:
    ThreadLocalRhiHolder();
    ~ThreadLocalRhiHolder() { resetRhi(); }

    QRhi *ensureRhi(QRhi *referenceRhi)
    {
        if (m_rhi || m_cpuOnly)
            return m_rhi.get();

        QRhi::Implementation referenceBackend = referenceRhi ? referenceRhi->backend() : QRhi::Null;
        const QPlatformIntegration *qpa = QGuiApplicationPrivate::platformIntegration();

        if (qpa && qpa->hasCapability(QPlatformIntegration::RhiBasedRendering)) {

#if QT_CONFIG(metal)
            if (referenceBackend == QRhi::Metal || referenceBackend == QRhi::Null) {
                QRhiMetalInitParams params;
                m_rhi.reset(QRhi::create(QRhi::Metal, &params));
            }
#endif

#if defined(Q_OS_WIN)
            if (referenceBackend == QRhi::D3D11 || referenceBackend == QRhi::Null) {
                QRhiD3D11InitParams params;
                m_rhi.reset(QRhi::create(QRhi::D3D11, &params));
            }
#endif

#if QT_CONFIG(opengl)
            if (!m_rhi && (referenceBackend == QRhi::OpenGLES2 || referenceBackend == QRhi::Null)) {
                if (qpa->hasCapability(QPlatformIntegration::OpenGL)
                    && qpa->hasCapability(QPlatformIntegration::RasterGLSurface)
                    && !QCoreApplication::testAttribute(Qt::AA_ForceRasterWidgets)) {

                    m_fallbackSurface.reset(QRhiGles2InitParams::newFallbackSurface());
                    QRhiGles2InitParams params;
                    params.fallbackSurface = m_fallbackSurface.get();
                    if (referenceBackend == QRhi::OpenGLES2)
                        params.shareContext = static_cast<const QRhiGles2NativeHandles *>(
                                                      referenceRhi->nativeHandles())
                                                      ->context;
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

private:
    void resetRhi()
    {
        m_rhi.reset();
#if QT_CONFIG(opengl)
        m_fallbackSurface.reset();
#endif
        m_cpuOnly = false;
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

Q_CONSTINIT thread_local std::optional<ThreadLocalRhiHolder> g_threadLocalRhiHolder;

ThreadLocalRhiHolder::ThreadLocalRhiHolder()
{
    if (QThread::isMainThread()) {
        // ensure cleanup in qApp dtor
        qAddPostRoutine([] {
            g_threadLocalRhiHolder.reset();
        });
    }
}

} // namespace

QRhi *qEnsureThreadLocalRhi(QRhi *referenceRhi)
{
    if (!g_threadLocalRhiHolder)
        g_threadLocalRhiHolder.emplace();

    return g_threadLocalRhiHolder->ensureRhi(referenceRhi);
}

QT_END_NAMESPACE
