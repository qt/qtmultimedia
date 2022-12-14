// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "androidsurfaceview_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtGui/qwindow.h>

QT_BEGIN_NAMESPACE

static const char QtSurfaceHolderCallbackClassName[] = "org/qtproject/qt/android/multimedia/QtSurfaceHolderCallback";
typedef QList<AndroidSurfaceHolder *> SurfaceHolders;
Q_GLOBAL_STATIC(SurfaceHolders, surfaceHolders)
Q_GLOBAL_STATIC(QMutex, shLock)

AndroidSurfaceHolder::AndroidSurfaceHolder(QJniObject object)
    : m_surfaceHolder(object)
    , m_surfaceCreated(false)
{
    if (!m_surfaceHolder.isValid())
        return;

    {
        QMutexLocker locker(shLock());
        surfaceHolders->append(this);
    }

    QJniObject callback(QtSurfaceHolderCallbackClassName, "(J)V", reinterpret_cast<jlong>(this));
    m_surfaceHolder.callMethod<void>("addCallback",
                                     "(Landroid/view/SurfaceHolder$Callback;)V",
                                     callback.object());
}

AndroidSurfaceHolder::~AndroidSurfaceHolder()
{
    QMutexLocker locker(shLock());
    const int i = surfaceHolders->indexOf(this);
    if (Q_UNLIKELY(i == -1))
        return;

    surfaceHolders->remove(i);
}

jobject AndroidSurfaceHolder::surfaceHolder() const
{
    return m_surfaceHolder.object();
}

bool AndroidSurfaceHolder::isSurfaceCreated() const
{
    QMutexLocker locker(shLock());
    return m_surfaceCreated;
}

void AndroidSurfaceHolder::handleSurfaceCreated(JNIEnv*, jobject, jlong id)
{
    QMutexLocker locker(shLock());
    const int i = surfaceHolders->indexOf(reinterpret_cast<AndroidSurfaceHolder *>(id));
    if (Q_UNLIKELY(i == -1))
        return;

    (*surfaceHolders)[i]->m_surfaceCreated = true;
    Q_EMIT (*surfaceHolders)[i]->surfaceCreated();
}

void AndroidSurfaceHolder::handleSurfaceDestroyed(JNIEnv*, jobject, jlong id)
{
    QMutexLocker locker(shLock());
    const int i = surfaceHolders->indexOf(reinterpret_cast<AndroidSurfaceHolder *>(id));
    if (Q_UNLIKELY(i == -1))
        return;

    (*surfaceHolders)[i]->m_surfaceCreated = false;
}

bool AndroidSurfaceHolder::registerNativeMethods()
{
    static const JNINativeMethod methods[] = {
        {"notifySurfaceCreated", "(J)V", (void *)AndroidSurfaceHolder::handleSurfaceCreated},
        {"notifySurfaceDestroyed", "(J)V", (void *)AndroidSurfaceHolder::handleSurfaceDestroyed}
    };

    const int size = std::size(methods);
    return QJniEnvironment().registerNativeMethods(QtSurfaceHolderCallbackClassName, methods, size);
}

AndroidSurfaceView::AndroidSurfaceView()
    : m_window(0)
    , m_surfaceHolder(0)
    , m_pendingVisible(-1)
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([this] {
        m_surfaceView = QJniObject("android/view/SurfaceView",
                                   "(Landroid/content/Context;)V",
                                   QNativeInterface::QAndroidApplication::context());
    }).waitForFinished();

    Q_ASSERT(m_surfaceView.isValid());

    QJniObject holder = m_surfaceView.callObjectMethod("getHolder",
                                                       "()Landroid/view/SurfaceHolder;");
    if (!holder.isValid()) {
        m_surfaceView = QJniObject();
    } else {
        m_surfaceHolder = new AndroidSurfaceHolder(holder);
        connect(m_surfaceHolder, &AndroidSurfaceHolder::surfaceCreated,
                this, &AndroidSurfaceView::surfaceCreated);
        { // Lock now to avoid a race with handleSurfaceCreated()
            QMutexLocker locker(shLock());
            m_window = QWindow::fromWinId(WId(m_surfaceView.object()));

            if (m_pendingVisible != -1)
                m_window->setVisible(m_pendingVisible);
            if (m_pendingGeometry.isValid())
                m_window->setGeometry(m_pendingGeometry);
        }
    }
}

AndroidSurfaceView::~AndroidSurfaceView()
{
    delete m_surfaceHolder;
    delete m_window;
}

AndroidSurfaceHolder *AndroidSurfaceView::holder() const
{
    return m_surfaceHolder;
}

void AndroidSurfaceView::setVisible(bool v)
{
    if (m_window)
        m_window->setVisible(v);
    else
        m_pendingVisible = int(v);
}

void AndroidSurfaceView::setGeometry(int x, int y, int width, int height)
{
    if (m_window)
        m_window->setGeometry(x, y, width, height);
    else
        m_pendingGeometry = QRect(x, y, width, height);
}

QT_END_NAMESPACE

#include "moc_androidsurfaceview_p.cpp"
