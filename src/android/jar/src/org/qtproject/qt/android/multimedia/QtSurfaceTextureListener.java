// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.multimedia;

import android.graphics.SurfaceTexture;

public class QtSurfaceTextureListener implements SurfaceTexture.OnFrameAvailableListener
{
    private final long m_id;

    public QtSurfaceTextureListener(long id)
    {
        m_id = id;
    }

    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture)
    {
        notifyFrameAvailable(m_id);
    }

    private static native void notifyFrameAvailable(long id);
}
