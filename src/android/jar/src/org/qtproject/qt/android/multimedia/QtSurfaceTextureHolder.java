// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.multimedia;

import android.view.SurfaceHolder;
import android.view.Surface;
import android.graphics.Rect;
import android.graphics.Canvas;

public class QtSurfaceTextureHolder implements SurfaceHolder
{
    private Surface surfaceTexture;

    public QtSurfaceTextureHolder(Surface surface)
    {
        surfaceTexture = surface;
    }

    @Override
    public void addCallback(SurfaceHolder.Callback callback)
    {
    }

    @Override
    public Surface getSurface()
    {
        return surfaceTexture;
    }

    @Override
    public Rect getSurfaceFrame()
    {
        return new Rect();
    }

    @Override
    public boolean isCreating()
    {
        return false;
    }

    @Override
    public Canvas lockCanvas(Rect dirty)
    {
        return new Canvas();
    }

    @Override
    public Canvas lockCanvas()
    {
        return new Canvas();
    }

    @Override
    public void removeCallback(SurfaceHolder.Callback callback)
    {
    }

    @Override
    public void setFixedSize(int width, int height)
    {
    }

    @Override
    public void setFormat(int format)
    {
    }

    @Override
    public void setKeepScreenOn(boolean screenOn)
    {
    }

    @Override
    public void setSizeFromLayout()
    {
    }

    @Override
    public void setType(int type)
    {
    }

    @Override
    public void unlockCanvasAndPost(Canvas canvas)
    {
    }
}
