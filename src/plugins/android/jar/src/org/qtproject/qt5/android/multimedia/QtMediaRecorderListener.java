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

import android.media.MediaRecorder;

public class QtMediaRecorderListener implements MediaRecorder.OnErrorListener, MediaRecorder.OnInfoListener
{
    private long m_id = -1;

    public QtMediaRecorderListener(long id)
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
