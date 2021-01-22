/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include <directshoweventloop.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qcoreevent.h>

QT_BEGIN_NAMESPACE

class DirectShowPostedEvent
{
    Q_DISABLE_COPY(DirectShowPostedEvent)
public:
    DirectShowPostedEvent(QObject *receiver, QEvent *event)
        : receiver(receiver)
        , event(event)
    {
    }

    ~DirectShowPostedEvent()
    {
        delete event;
    }

    QObject *receiver;
    QEvent *event;
    DirectShowPostedEvent *next = nullptr;
};

DirectShowEventLoop::DirectShowEventLoop(QObject *parent)
    : QObject(parent)
    , m_eventHandle(::CreateEvent(nullptr, FALSE, FALSE, nullptr))
    , m_waitHandle(::CreateEvent(nullptr, FALSE, FALSE, nullptr))
{
}

DirectShowEventLoop::~DirectShowEventLoop()
{
    ::CloseHandle(m_eventHandle);
    ::CloseHandle(m_waitHandle);

    for (DirectShowPostedEvent *post = m_postsHead; post; post = m_postsHead) {
        m_postsHead = m_postsHead->next;

        delete post;
    }
}

void DirectShowEventLoop::wait(QMutex *mutex)
{
    ::ResetEvent(m_waitHandle);

    mutex->unlock();

    HANDLE handles[] = { m_eventHandle, m_waitHandle };
    while (::WaitForMultipleObjects(2, handles, false, INFINITE) == WAIT_OBJECT_0)
        processEvents();

    mutex->lock();
}

void DirectShowEventLoop::wake()
{
    ::SetEvent(m_waitHandle);
}

void DirectShowEventLoop::postEvent(QObject *receiver, QEvent *event)
{
    QMutexLocker locker(&m_mutex);

    DirectShowPostedEvent *post = new DirectShowPostedEvent(receiver, event);

    if (m_postsTail)
        m_postsTail->next = post;
    else
        m_postsHead = post;

    m_postsTail = post;

    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
    ::SetEvent(m_eventHandle);
}

void DirectShowEventLoop::customEvent(QEvent *event)
{
    if (event->type() == QEvent::User) {
        processEvents();
    } else {
        QObject::customEvent(event);
    }
}

void DirectShowEventLoop::processEvents()
{
    QMutexLocker locker(&m_mutex);

    ::ResetEvent(m_eventHandle);

    while(m_postsHead) {
        DirectShowPostedEvent *post = m_postsHead;
        m_postsHead = m_postsHead->next;

        if (!m_postsHead)
            m_postsTail = nullptr;

        locker.unlock();
        QCoreApplication::sendEvent(post->receiver, post->event);
        delete post;
        locker.relock();
    }
}

QT_END_NAMESPACE
