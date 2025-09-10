// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.multimedia;

import android.media.MediaRecorder;

class QtMediaRecorderListener implements MediaRecorder.OnErrorListener, MediaRecorder.OnInfoListener
{
    private long m_id = -1;

    QtMediaRecorderListener(long id)
    {
        m_id = id;
    }

    @Override
    public void onError(MediaRecorder mr, int what, int extra)
    {
        notifyError(m_id, what, extra);
    }

    @Override
    public void onInfo(MediaRecorder mr, int what, int extra)
    {
        notifyInfo(m_id, what, extra);
    }

    private static native void notifyError(long id, int what, int extra);
    private static native void notifyInfo(long id, int what, int extra);
}
