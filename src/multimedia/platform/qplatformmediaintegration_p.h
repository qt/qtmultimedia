/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/
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

class Q_MULTIMEDIA_EXPORT QPlatformMediaIntegration
{
public:
    static QPlatformMediaIntegration *instance();

    // API to be able to test with a mock backend
    static void setIntegration(QPlatformMediaIntegration *);

    virtual ~QPlatformMediaIntegration();
    virtual QPlatformMediaDevices *devices() = 0;
    virtual QPlatformMediaFormatInfo *formatInfo() = 0;

    virtual QPlatformAudioDecoder *createAudioDecoder(QAudioDecoder *) { return nullptr; }
    virtual QPlatformMediaCaptureSession *createCaptureSession() { return nullptr; }
    virtual QPlatformMediaPlayer *createPlayer(QMediaPlayer *) { return nullptr; }
    virtual QPlatformCamera *createCamera(QCamera *) { return nullptr; }
    virtual QPlatformMediaRecorder *createRecorder(QMediaRecorder *) { return nullptr; }
    virtual QPlatformImageCapture *createImageCapture(QImageCapture *) { return nullptr; }

    virtual QPlatformAudioInput *createAudioInput(QAudioInput *);
    virtual QPlatformAudioOutput *createAudioOutput(QAudioOutput *);

    virtual QPlatformVideoSink *createVideoSink(QVideoSink *) { return nullptr; }
};

QT_END_NAMESPACE


#endif // QPLATFORMMEDIAINTERFACE_H
