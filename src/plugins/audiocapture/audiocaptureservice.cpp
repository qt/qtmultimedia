/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "audiocaptureservice.h"
#include "audiocapturesession.h"
#include "audioendpointselector.h"
#include "audioencodercontrol.h"
#include "audiocontainercontrol.h"
#include "audiomediarecordercontrol.h"

AudioCaptureService::AudioCaptureService(QObject *parent):
    QMediaService(parent)
{
    m_session = new AudioCaptureSession(this);
    m_encoderControl  = new AudioEncoderControl(m_session);
    m_containerControl = new AudioContainerControl(m_session);
    m_mediaControl   = new AudioMediaRecorderControl(m_session);
    m_endpointSelector  = new AudioEndpointSelector(m_session);
}

AudioCaptureService::~AudioCaptureService()
{
    delete m_encoderControl;
    delete m_containerControl;
    delete m_endpointSelector;
    delete m_mediaControl;
    delete m_session;
}

QMediaControl *AudioCaptureService::requestControl(const char *name)
{
    if (qstrcmp(name,QMediaRecorderControl_iid) == 0)
        return m_mediaControl;

    if (qstrcmp(name,QAudioEncoderControl_iid) == 0)
        return m_encoderControl;

    if (qstrcmp(name,QAudioEndpointSelector_iid) == 0)
        return m_endpointSelector;

    if (qstrcmp(name,QMediaContainerControl_iid) == 0)
        return m_containerControl;

    return 0;
}

void AudioCaptureService::releaseControl(QMediaControl *control)
{
    Q_UNUSED(control)
}


