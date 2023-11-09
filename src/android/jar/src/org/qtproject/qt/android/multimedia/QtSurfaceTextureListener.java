// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
