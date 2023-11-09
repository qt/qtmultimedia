// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

package org.qtproject.qt.android.multimedia;

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
