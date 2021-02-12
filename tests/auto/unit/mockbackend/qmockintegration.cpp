/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmockintegration_p.h"
#include "qmockdevicemanager_p.h"
#include "mockmediaplayer.h"
#include "mockaudiodecodercontrol.h"
#include "mockmediarecorderservice.h"

QT_BEGIN_NAMESPACE

QMockIntegration::QMockIntegration()
{
    setIntegration(this);
}

QMockIntegration::~QMockIntegration()
{
    setIntegration(nullptr);
    delete m_manager;
}

QMediaPlatformDeviceManager *QMockIntegration::deviceManager()
{
    if (!m_manager)
        m_manager = new QMockDeviceManager();
    return m_manager;
}

QAudioDecoderControl *QMockIntegration::createAudioDecoder()
{
    if (m_flags & NoAudioDecoderInterface)
        m_lastAudioDecoderControl = nullptr;
    else
        m_lastAudioDecoderControl = new MockAudioDecoderControl;
    return m_lastAudioDecoderControl;
}

QPlatformMediaPlayer *QMockIntegration::createPlayer()
{
    if (m_flags & NoPlayerInterface)
        m_lastPlayer = nullptr;
    else
        m_lastPlayer = new MockMediaPlayer;
    return m_lastPlayer;
}

QMediaPlatformCaptureInterface *QMockIntegration::createCaptureInterface(QMediaRecorder::CaptureMode mode)
{
    Q_UNUSED(mode);
    if (m_flags & NoCaptureInterface)
        m_lastCaptureService = nullptr;
    else
        m_lastCaptureService = new MockMediaRecorderService();
    return m_lastCaptureService;
}

bool MockMediaRecorderService::simpleCamera = false;

QT_END_NAMESPACE
