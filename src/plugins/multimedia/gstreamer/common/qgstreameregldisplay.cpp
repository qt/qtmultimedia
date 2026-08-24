// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreameregldisplay_p.h"

#if QT_CONFIG(gstreamer_gl) && QT_CONFIG(gstreamer_gl_egl)

#include <QtCore/qapplicationstatic.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpa/qplatformnativeinterface.h>

#if QT_CONFIG(linux_dmabuf)
#  include <QtMultimedia/private/qdmabuftextureimporter_p.h>
#endif

QT_BEGIN_NAMESPACE

struct EglHolder
{
    QMutex mutex;
    Qt::HANDLE display = nullptr;

    void queryDisplay()
    {
        Q_ASSERT(QThread::isMainThread());
        using namespace Qt::StringLiterals;
        auto *pni = qGuiApp ? qGuiApp->platformNativeInterface() : nullptr;
        display = pni ? pni->nativeResourceForIntegration("egldisplay"_ba) : nullptr;
    }
};

Q_APPLICATION_STATIC(EglHolder, s_eglHolder);

Qt::HANDLE qGstEglDisplay()
{
    EglHolder *holder = s_eglHolder();
    QMutexLocker lock(&holder->mutex);
    if (!holder->display && QThread::isMainThread())
        holder->queryDisplay();
    return holder->display;
}

#  if QT_CONFIG(linux_dmabuf)
bool qGstEglCanMapDmaBuf()
{
    using namespace QtMultimediaPrivate;

    EglHolder *holder = s_eglHolder();
    QMutexLocker lock(&holder->mutex);
    if (!holder->display && QThread::isMainThread())
        holder->queryDisplay();
    return (holder->display && QEglImageFunctions::instance().isValid());
}
#  endif

QT_END_NAMESPACE

#endif // QT_CONFIG(gstreamer_gl) && QT_CONFIG(gstreamer_gl_egl)
