// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QPLATFORMMEDIAINTEGRATION_H
#define QPLATFORMMEDIAINTEGRATION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtmultimediaglobal_p.h>
#include <qmediarecorder.h>

QT_BEGIN_NAMESPACE

class QMediaPlayer;
class QAudioDecoder;
class QCamera;
class QMediaRecorder;
class QImageCapture;
class QMediaDevices;
class QPlatformMediaDevices;
class QPlatformMediaCaptureSession;
class QPlatformMediaPlayer;
class QPlatformAudioDecoder;
class QPlatformCamera;
class QPlatformMediaRecorder;
class QPlatformImageCapture;
class QPlatformMediaFormatInfo;
class QObject;
class QPlatformVideoSink;
class QVideoSink;
class QAudioInput;
class QAudioOutput;
class QPlatformAudioInput;
class QPlatformAudioOutput;
class QPlatformVideoDevices;

class Q_MULTIMEDIA_EXPORT QPlatformMediaIntegration
{
public:
    static QPlatformMediaIntegration *instance();

    // API to be able to test with a mock backend
    static void setIntegration(QPlatformMediaIntegration *);

    virtual ~QPlatformMediaIntegration();
    virtual QPlatformMediaFormatInfo *formatInfo() = 0;

    virtual QList<QCameraDevice> videoInputs();
    virtual QPlatformCamera *createCamera(QCamera *) { return nullptr; }

    virtual QPlatformAudioDecoder *createAudioDecoder(QAudioDecoder *) { return nullptr; }
    virtual QPlatformMediaCaptureSession *createCaptureSession() { return nullptr; }
    virtual QPlatformMediaPlayer *createPlayer(QMediaPlayer *) { return nullptr; }
    virtual QPlatformMediaRecorder *createRecorder(QMediaRecorder *) { return nullptr; }
    virtual QPlatformImageCapture *createImageCapture(QImageCapture *) { return nullptr; }

    virtual QPlatformAudioInput *createAudioInput(QAudioInput *);
    virtual QPlatformAudioOutput *createAudioOutput(QAudioOutput *);

    virtual QPlatformVideoSink *createVideoSink(QVideoSink *) { return nullptr; }

protected:
    QPlatformVideoDevices *m_videoDevices = nullptr;
};

QT_END_NAMESPACE


#endif // QPLATFORMMEDIAINTERFACE_H
