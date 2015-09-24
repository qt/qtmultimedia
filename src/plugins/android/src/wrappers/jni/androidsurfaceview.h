/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef ANDROIDSURFACEVIEW_H
#define ANDROIDSURFACEVIEW_H

#include <QtCore/private/qjni_p.h>
#include <qrect.h>
#include <QtCore/qrunnable.h>

QT_BEGIN_NAMESPACE

class QWindow;

class AndroidSurfaceHolder : public QObject
{
    Q_OBJECT
public:
    ~AndroidSurfaceHolder();

    jobject surfaceHolder() const;
    bool isSurfaceCreated() const;

    static bool initJNI(JNIEnv *env);

Q_SIGNALS:
    void surfaceCreated();

private:
    AndroidSurfaceHolder(QJNIObjectPrivate object);

    static void handleSurfaceCreated(JNIEnv*, jobject, jlong id);
    static void handleSurfaceDestroyed(JNIEnv*, jobject, jlong id);

    QJNIObjectPrivate m_surfaceHolder;
    bool m_surfaceCreated;

    friend class AndroidSurfaceView;
};

class AndroidSurfaceView : public QObject, public QRunnable
{
    Q_OBJECT
public:
    AndroidSurfaceView();
    ~AndroidSurfaceView();

    AndroidSurfaceHolder *holder() const;

    void setVisible(bool v);
    void setGeometry(int x, int y, int width, int height);

    bool event(QEvent *);

Q_SIGNALS:
    void surfaceCreated();

protected:
    void run() override;

private:
    QJNIObjectPrivate m_surfaceView;
    QWindow *m_window;
    AndroidSurfaceHolder *m_surfaceHolder;
    int m_pendingVisible;
    QRect m_pendingGeometry;
};

QT_END_NAMESPACE

#endif // ANDROIDSURFACEVIEW_H
