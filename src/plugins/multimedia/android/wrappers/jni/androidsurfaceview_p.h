// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROIDSURFACEVIEW_H
#define ANDROIDSURFACEVIEW_H

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

#include <QtCore/qjniobject.h>
#include <qrect.h>
#include <QtCore/qrunnable.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QWindow;

class AndroidSurfaceHolder : public QObject
{
    Q_OBJECT
public:
    ~AndroidSurfaceHolder();

    jobject surfaceHolder() const;
    bool isSurfaceCreated() const;

    static bool registerNativeMethods();

Q_SIGNALS:
    void surfaceCreated();

private:
    AndroidSurfaceHolder(QJniObject object);

    static void handleSurfaceCreated(JNIEnv*, jobject, jlong id);
    static void handleSurfaceDestroyed(JNIEnv*, jobject, jlong id);

    QJniObject m_surfaceHolder;
    bool m_surfaceCreated;

    friend class AndroidSurfaceView;
};

class AndroidSurfaceView : public QObject
{
    Q_OBJECT
public:
    AndroidSurfaceView();
    ~AndroidSurfaceView();

    AndroidSurfaceHolder *holder() const;

    void setVisible(bool v);
    void setGeometry(int x, int y, int width, int height);

Q_SIGNALS:
    void surfaceCreated();

private:
    QJniObject m_surfaceView;
    QWindow *m_window;
    AndroidSurfaceHolder *m_surfaceHolder;
    int m_pendingVisible;
    QRect m_pendingGeometry;
};

QT_END_NAMESPACE

#endif // ANDROIDSURFACEVIEW_H
