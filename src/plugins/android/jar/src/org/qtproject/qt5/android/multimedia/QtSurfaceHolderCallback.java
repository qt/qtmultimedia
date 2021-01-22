/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtMultimedia of the Qt Toolkit.
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

package org.qtproject.qt5.android.multimedia;

import android.view.SurfaceHolder;

public class QtSurfaceHolderCallback implements SurfaceHolder.Callback
{
    private long m_id = -1;

    public QtSurfaceHolderCallback(long id)
    {
        m_id = id;
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
    {
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder)
    {
        notifySurfaceCreated(m_id);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        notifySurfaceDestroyed(m_id);
    }


    private static native void notifySurfaceCreated(long id);
    private static native void notifySurfaceDestroyed(long id);
}
