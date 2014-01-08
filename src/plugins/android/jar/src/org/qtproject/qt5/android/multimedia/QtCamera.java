/****************************************************************************
 **
 ** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
 ** Contact: http://www.qt-project.org/legal
 **
 ** This file is part of the QtMultimedia module of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:LGPL$
 ** Commercial License Usage
 ** Licensees holding valid commercial Qt licenses may use this file in
 ** accordance with the commercial license agreement provided with the
 ** Software or, alternatively, in accordance with the terms contained in
 ** a written agreement between you and Digia.  For licensing terms and
 ** conditions see http://qt.digia.com/licensing.  For further information
 ** use the contact form at http://qt.digia.com/contact-us.
 **
 ** GNU Lesser General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU Lesser
 ** General Public License version 2.1 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.LGPL included in the
 ** packaging of this file.  Please review the following information to
 ** ensure the GNU Lesser General Public License version 2.1 requirements
 ** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 **
 ** In addition, as a special exception, Digia gives you certain additional
 ** rights.  These rights are described in the Digia Qt LGPL Exception
 ** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
 **
 ** GNU General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU
 ** General Public License version 3.0 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.GPL included in the
 ** packaging of this file.  Please review the following information to
 ** ensure the GNU General Public License version 3.0 requirements will be
 ** met: http://www.gnu.org/copyleft/gpl.html.
 **
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
import java.util.concurrent.locks.ReentrantLock;

public class QtCamera implements Camera.ShutterCallback,
                                 Camera.PictureCallback,
                                 Camera.AutoFocusCallback,
                                 Camera.PreviewCallback
{
    private int m_cameraId = -1;
    private Camera m_camera = null;
    private byte[] m_cameraPreviewFirstBuffer = null;
    private byte[] m_cameraPreviewSecondBuffer = null;
    private int m_actualPreviewBuffer = 0;
    private final ReentrantLock m_buffersLock = new ReentrantLock();
    private boolean m_isReleased = false;
    private boolean m_fetchEachFrame = false;

    private static final String TAG = "Qt Camera";

    private QtCamera(int id, Camera cam)
    {
        m_cameraId = id;
        m_camera = cam;
    }

    public static QtCamera open(int cameraId)
    {
        try {
            Camera cam = Camera.open(cameraId);
            return new QtCamera(cameraId, cam);
        } catch(Exception e) {
            Log.d(TAG, e.getMessage());
        }
        return null;
    }

    public Camera.Parameters getParameters()
    {
        return m_camera.getParameters();
    }

    public void lock()
    {
        try {
            m_camera.lock();
        } catch(Exception e) {
            Log.d(TAG, e.getMessage());
        }
    }

    public void unlock()
    {
        try {
            m_camera.unlock();
        } catch(Exception e) {
            Log.d(TAG, e.getMessage());
        }
    }

    public void release()
    {
        m_isReleased = true;
        m_camera.release();
    }

    public void reconnect()
    {
        try {
            m_camera.reconnect();
        } catch(Exception e) {
            Log.d(TAG, e.getMessage());
        }
    }

    public void setDisplayOrientation(int degrees)
    {
        m_camera.setDisplayOrientation(degrees);
    }

    public void setParameters(Camera.Parameters params)
    {
        try {
            m_camera.setParameters(params);
        } catch(Exception e) {
            Log.d(TAG, e.getMessage());
        }
    }

    public void setPreviewTexture(SurfaceTexture surfaceTexture)
    {
        try {
            m_camera.setPreviewTexture(surfaceTexture);
        } catch(Exception e) {
            Log.d(TAG, e.getMessage());
        }
    }

    public void fetchEachFrame(boolean fetch)
    {
        m_fetchEachFrame = fetch;
    }

    public void startPreview()
    {
        Camera.Size previewSize = m_camera.getParameters().getPreviewSize();
        double bytesPerPixel = ImageFormat.getBitsPerPixel(m_camera.getParameters().getPreviewFormat()) / 8.0;
        int bufferSizeNeeded = (int)Math.ceil(bytesPerPixel*previewSize.width*previewSize.height);

        //We need to clear preview buffers queue here, but there is no method to do it
        //Though just resetting preview callback do the trick
        m_camera.setPreviewCallback(null);
        m_buffersLock.lock();
        if (m_cameraPreviewFirstBuffer == null || m_cameraPreviewFirstBuffer.length < bufferSizeNeeded)
            m_cameraPreviewFirstBuffer = new byte[bufferSizeNeeded];
        if (m_cameraPreviewSecondBuffer == null || m_cameraPreviewSecondBuffer.length < bufferSizeNeeded)
            m_cameraPreviewSecondBuffer = new byte[bufferSizeNeeded];
        addCallbackBuffer();
        m_buffersLock.unlock();
        m_camera.setPreviewCallbackWithBuffer(this);

        m_camera.startPreview();
    }

    public void stopPreview()
    {
        m_camera.stopPreview();
    }

    public void autoFocus()
    {
        m_camera.autoFocus(this);
    }

    public void cancelAutoFocus()
    {
        m_camera.cancelAutoFocus();
    }

    public void takePicture()
    {
        try {
            m_camera.takePicture(this, null, this);
        } catch(Exception e) {
            Log.d(TAG, e.getMessage());
        }
    }

    public byte[] lockAndFetchPreviewBuffer()
    {
        //This method should always be followed by unlockPreviewBuffer()
        //This method is not just a getter. It also marks last preview as already seen one.
        //We should reset actualBuffer flag here to make sure we will not use old preview with future captures
        byte[] result = null;
        m_buffersLock.lock();
        if (m_actualPreviewBuffer == 1)
            result = m_cameraPreviewFirstBuffer;
        else if (m_actualPreviewBuffer == 2)
            result = m_cameraPreviewSecondBuffer;
        m_actualPreviewBuffer = 0;
        return result;
    }

    public void unlockPreviewBuffer()
    {
        if (m_buffersLock.isHeldByCurrentThread())
            m_buffersLock.unlock();
    }

    private void addCallbackBuffer()
    {
        if (m_isReleased)
            return;

        m_camera.addCallbackBuffer((m_actualPreviewBuffer == 1)
                                    ? m_cameraPreviewSecondBuffer
                                    : m_cameraPreviewFirstBuffer);
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
    public void onPreviewFrame(byte[] data, Camera camera)
    {
        m_buffersLock.lock();

        if (data != null && m_fetchEachFrame)
            notifyFrameFetched(m_cameraId, data);

        if (data == m_cameraPreviewFirstBuffer)
            m_actualPreviewBuffer = 1;
        else if (data == m_cameraPreviewSecondBuffer)
            m_actualPreviewBuffer = 2;
        else
            m_actualPreviewBuffer = 0;
        addCallbackBuffer();
        m_buffersLock.unlock();
    }

    @Override
    public void onAutoFocus(boolean success, Camera camera)
    {
        notifyAutoFocusComplete(m_cameraId, success);
    }

    private static native void notifyAutoFocusComplete(int id, boolean success);
    private static native void notifyPictureExposed(int id);
    private static native void notifyPictureCaptured(int id, byte[] data);
    private static native void notifyFrameFetched(int id, byte[] data);
}
