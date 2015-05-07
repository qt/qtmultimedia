/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtMultimedia of the Qt Toolkit.
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

package org.qtproject.qt5.android.multimedia;

import android.hardware.Camera;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.util.Log;
import java.lang.Math;

public class QtCameraListener implements Camera.ShutterCallback,
                                         Camera.PictureCallback,
                                         Camera.AutoFocusCallback,
                                         Camera.PreviewCallback
{
    private static final String TAG = "Qt Camera";

    private static final int BUFFER_POOL_SIZE = 2;

    private int m_cameraId = -1;

    private boolean m_notifyNewFrames = false;
    private byte[][] m_previewBuffers = null;
    private byte[] m_lastPreviewBuffer = null;
    private Camera.Size m_previewSize = null;

    private QtCameraListener(int id)
    {
        m_cameraId = id;
    }

    public void notifyNewFrames(boolean notify)
    {
        m_notifyNewFrames = notify;
    }

    public byte[] lastPreviewBuffer()
    {
        return m_lastPreviewBuffer;
    }

    public int previewWidth()
    {
        if (m_previewSize == null)
            return -1;

        return m_previewSize.width;
    }

    public int previewHeight()
    {
        if (m_previewSize == null)
            return -1;

        return m_previewSize.height;
    }

    public void setupPreviewCallback(Camera camera)
    {
        // Clear previous callback (also clears added buffers)
        m_lastPreviewBuffer = null;
        camera.setPreviewCallbackWithBuffer(null);

        final Camera.Parameters params = camera.getParameters();
        m_previewSize = params.getPreviewSize();
        double bytesPerPixel = ImageFormat.getBitsPerPixel(params.getPreviewFormat()) / 8.0;
        int bufferSizeNeeded = (int) Math.ceil(bytesPerPixel * m_previewSize.width * m_previewSize.height);

        // We could keep the same buffers when they are already bigger than the required size
        // but the Android doc says the size must match, so in doubt just replace them.
        if (m_previewBuffers == null || m_previewBuffers[0].length != bufferSizeNeeded)
            m_previewBuffers = new byte[BUFFER_POOL_SIZE][bufferSizeNeeded];

        // Add callback and queue all buffers
        camera.setPreviewCallbackWithBuffer(this);
        for (byte[] buffer : m_previewBuffers)
            camera.addCallbackBuffer(buffer);
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera)
    {
        // Re-enqueue the last buffer
        if (m_lastPreviewBuffer != null)
            camera.addCallbackBuffer(m_lastPreviewBuffer);

        m_lastPreviewBuffer = data;

        if (data != null && m_notifyNewFrames)
            notifyNewPreviewFrame(m_cameraId, data, m_previewSize.width, m_previewSize.height);
    }

    @Override
    public void onShutter()
    {
        notifyPictureExposed(m_cameraId);
    }

    @Override
    public void onPictureTaken(byte[] data, Camera camera)
    {
        notifyPictureCaptured(m_cameraId, data);
    }

    @Override
    public void onAutoFocus(boolean success, Camera camera)
    {
        notifyAutoFocusComplete(m_cameraId, success);
    }

    private static native void notifyAutoFocusComplete(int id, boolean success);
    private static native void notifyPictureExposed(int id);
    private static native void notifyPictureCaptured(int id, byte[] data);
    private static native void notifyNewPreviewFrame(int id, byte[] data, int width, int height);
}
