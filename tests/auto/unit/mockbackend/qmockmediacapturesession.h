/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMOCKMEDIACAPTURESESSION_H
#define QMOCKMEDIACAPTURESESSION_H

#include "qmockmediaencoder.h"
#include "qmockimagecapture.h"
#include "qmockcamera.h"
#include "qmockimagecapture.h"
#include <private/qplatformmediacapture_p.h>

QT_BEGIN_NAMESPACE

class QMockMediaCaptureSession : public QPlatformMediaCaptureSession
{
    Q_OBJECT
public:
    QMockMediaCaptureSession()
        : hasControls(true)
    {
    }
    ~QMockMediaCaptureSession()
    {
    }

    QPlatformCamera *camera() override { return hasControls ? mockCameraControl : nullptr; }

    void setCamera(QPlatformCamera *camera) override
    {
        QMockCamera *control = static_cast<QMockCamera *>(camera);
        if (mockCameraControl == control)
            return;

        mockCameraControl = control;
    }

    void setImageCapture(QPlatformImageCapture *imageCapture) override
    {
        mockImageCapture = imageCapture;
    }
    QPlatformImageCapture *imageCapture() override { return hasControls ? mockImageCapture : nullptr; }

    QPlatformMediaRecorder *mediaRecorder() override { return hasControls ? mockControl : nullptr; }
    void setMediaRecorder(QPlatformMediaRecorder *recorder) override
    {
        if (!hasControls) {
            mockControl = nullptr;
            return;
        }
        QMockMediaEncoder *control = static_cast<QMockMediaEncoder *>(recorder);
        if (mockControl == control)
            return;

        mockControl = control;
    }

    void setVideoPreview(QVideoSink *) override {}

    void setAudioInput(QPlatformAudioInput *input) override
    {
        m_audioInput = input;
    }

    QMockCamera *mockCameraControl = nullptr;
    QPlatformImageCapture *mockImageCapture = nullptr;
    QMockMediaEncoder *mockControl = nullptr;
    QPlatformAudioInput *m_audioInput = nullptr;
    bool hasControls;
};

QT_END_NAMESPACE

#endif // QMOCKMEDIACAPTURESESSION_H
