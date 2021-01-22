/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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

#ifndef AUDIOCAPTURESERVICE_H
#define AUDIOCAPTURESERVICE_H

#include <QtCore/qobject.h>

#include "qmediaservice.h"

QT_BEGIN_NAMESPACE

class AudioCaptureSession;
class AudioEncoderControl;
class AudioContainerControl;
class AudioMediaRecorderControl;
class AudioInputSelector;

class AudioCaptureService : public QMediaService
{
    Q_OBJECT
public:
    AudioCaptureService(QObject *parent = 0);
    ~AudioCaptureService();

    QMediaControl *requestControl(const char *interface);
    void releaseControl(QMediaControl *control);
private:
    AudioCaptureSession       *m_session;
    AudioEncoderControl       *m_encoderControl;
    AudioContainerControl     *m_containerControl;
    AudioInputSelector        *m_inputSelector;
    AudioMediaRecorderControl *m_mediaControl;
};

QT_END_NAMESPACE

#endif
