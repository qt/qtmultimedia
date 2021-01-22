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

#ifndef DIRECTSHOWEVENTLOOP_H
#define DIRECTSHOWEVENTLOOP_H

#include <QtCore/qmutex.h>
#include <QtCore/qobject.h>
#include <QtCore/qwaitcondition.h>

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

class DirectShowPostedEvent;

class DirectShowEventLoop : public QObject
{
    Q_OBJECT
public:
    DirectShowEventLoop(QObject *parent = nullptr);
    ~DirectShowEventLoop() override;

    void wait(QMutex *mutex);
    void wake();

    void postEvent(QObject *object, QEvent *event);

protected:
    void customEvent(QEvent *event) override;

private:
    void processEvents();

    DirectShowPostedEvent *m_postsHead = nullptr;
    DirectShowPostedEvent *m_postsTail = nullptr;
    HANDLE m_eventHandle;
    HANDLE m_waitHandle;
    QMutex m_mutex;
};

QT_END_NAMESPACE

#endif
