// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.multimedia;

import android.view.SurfaceHolder;

class QtSurfaceHolderCallback implements SurfaceHolder.Callback
{
    private long m_id = -1;

    QtSurfaceHolderCallback(long id)
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
