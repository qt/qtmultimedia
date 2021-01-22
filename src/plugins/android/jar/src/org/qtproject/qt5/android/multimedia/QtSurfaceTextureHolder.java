/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the (whatever) of the Qt Toolkit.
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
