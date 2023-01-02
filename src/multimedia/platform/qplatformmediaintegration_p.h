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
#include <private/qmultimediautils_p.h>
#include <qmediarecorder.h>
#include <qstring.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QMediaPlayer;
class QAudioDecoder;
class QCamera;
class QScreenCapture;
class QMediaRecorder;
class QImageCapture;
class QMediaDevices;
class QPlatformMediaDevices;
class QPlatformMediaCaptureSession;
class QPlatformMediaPlayer;
class QPlatformAudioDecoder;
class QPlatformCamera;
class QPlatformScreenCapture;
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
    inline static const QString notAvailable = QStringLiteral("Not available");
public:
    static QPlatformMediaIntegration *instance();

    // API to be able to test with a mock backend
    static void setIntegration(QPlatformMediaIntegration *);

    QPlatformMediaIntegration();
    virtual ~QPlatformMediaIntegration();
    virtual QPlatformMediaFormatInfo *formatInfo() = 0;

    virtual QList<QCameraDevice> videoInputs();
    virtual QMaybe<QPlatformCamera *> createCamera(QCamera *) { return notAvailable; }
    virtual QPlatformScreenCapture *createScreenCapture(QScreenCapture *) { return nullptr; }

    virtual QMaybe<QPlatformAudioDecoder *> createAudioDecoder(QAudioDecoder *) { return notAvailable; }
    virtual QMaybe<QPlatformMediaCaptureSession *> createCaptureSession() { return notAvailable; }
    virtual QMaybe<QPlatformMediaPlayer *> createPlayer(QMediaPlayer *) { return notAvailable; }
    virtual QMaybe<QPlatformMediaRecorder *> createRecorder(QMediaRecorder *) { return notAvailable; }
    virtual QMaybe<QPlatformImageCapture *> createImageCapture(QImageCapture *) { return notAvailable; }

    virtual QMaybe<QPlatformAudioInput *> createAudioInput(QAudioInput *);
    virtual QMaybe<QPlatformAudioOutput *> createAudioOutput(QAudioOutput *);

    virtual QMaybe<QPlatformVideoSink *> createVideoSink(QVideoSink *) { return notAvailable; }

protected:
    std::unique_ptr<QPlatformVideoDevices> m_videoDevices;
};

QT_END_NAMESPACE


#endif // QPLATFORMMEDIAINTERFACE_H
