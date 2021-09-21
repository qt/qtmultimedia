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

#ifndef QMOCKINTEGRATION_H
#define QMOCKINTEGRATION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qplatformmediaintegration_p.h>

QT_BEGIN_NAMESPACE

class QMockMediaDevices;
class QMockMediaPlayer;
class QMockAudioDecoder;
class QMockCamera;
class QMockMediaCaptureSession;
class QMockVideoSink;

class QMockIntegration : public QPlatformMediaIntegration
{
public:
    QMockIntegration();
    ~QMockIntegration();

    QPlatformMediaDevices *devices() override;
    QPlatformMediaFormatInfo *formatInfo() override { return nullptr; }

    QPlatformAudioDecoder *createAudioDecoder(QAudioDecoder *decoder) override;
    QPlatformMediaPlayer *createPlayer(QMediaPlayer *) override;
    QPlatformCamera *createCamera(QCamera *) override;
    QPlatformMediaRecorder *createRecorder(QMediaRecorder *) override;
    QPlatformImageCapture *createImageCapture(QImageCapture *) override;
    QPlatformMediaCaptureSession *createCaptureSession() override;
    QPlatformVideoSink *createVideoSink(QVideoSink *) override;

    QPlatformAudioOutput *createAudioOutput(QAudioOutput *) override;

    enum Flag {
        NoPlayerInterface = 0x1,
        NoAudioDecoderInterface = 0x2,
        NoCaptureInterface = 0x4
    };
    Q_DECLARE_FLAGS(Flags, Flag);

    void setFlags(Flags f) { m_flags = f; }
    Flags flags() const { return m_flags; }

    QMockMediaPlayer *lastPlayer() const { return m_lastPlayer; }
    QMockAudioDecoder *lastAudioDecoder() const { return m_lastAudioDecoderControl; }
    QMockCamera *lastCamera() const { return m_lastCamera; }
    // QMockMediaEncoder *lastEncoder const { return m_lastEncoder; }
    QMockMediaCaptureSession *lastCaptureService() const { return m_lastCaptureService; }
    QMockVideoSink *lastVideoSink() const { return m_lastVideoSink; }

private:
    Flags m_flags = {};
    QMockMediaDevices *m_devices = nullptr;
    QMockMediaPlayer *m_lastPlayer = nullptr;
    QMockAudioDecoder *m_lastAudioDecoderControl = nullptr;
    QMockCamera *m_lastCamera = nullptr;
    // QMockMediaEncoder *m_lastEncoder = nullptr;
    QMockMediaCaptureSession *m_lastCaptureService = nullptr;
    QMockVideoSink *m_lastVideoSink;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMockIntegration::Flags);

QT_END_NAMESPACE

#endif
