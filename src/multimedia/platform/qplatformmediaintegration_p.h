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
#include <qcapturablewindow.h>
#include <qmediarecorder.h>
#include <qstring.h>

#include <memory>
#include <mutex>

QT_BEGIN_NAMESPACE

class QMediaPlayer;
class QAudioDecoder;
class QCamera;
class QScreenCapture;
class QWindowCapture;
class QMediaRecorder;
class QImageCapture;
class QMediaDevices;
class QPlatformMediaDevices;
class QPlatformMediaCaptureSession;
class QPlatformMediaPlayer;
class QPlatformAudioDecoder;
class QPlatformCamera;
class QPlatformSurfaceCapture;
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
class QCapturableWindow;
class QPlatformCapturableWindows;

class Q_MULTIMEDIA_EXPORT QPlatformMediaIntegration
{
    inline static const QString notAvailable = QStringLiteral("Not available");
public:
    static QPlatformMediaIntegration *instance();

    QPlatformMediaIntegration();
    virtual ~QPlatformMediaIntegration();
    const QPlatformMediaFormatInfo *formatInfo();

    virtual QList<QCameraDevice> videoInputs();
    virtual QMaybe<QPlatformCamera *> createCamera(QCamera *) { return notAvailable; }
    virtual QPlatformSurfaceCapture *createScreenCapture(QScreenCapture *) { return nullptr; }
    virtual QPlatformSurfaceCapture *createWindowCapture(QWindowCapture *) { return nullptr; }

    virtual QMaybe<QPlatformAudioDecoder *> createAudioDecoder(QAudioDecoder *) { return notAvailable; }
    virtual QMaybe<QPlatformMediaCaptureSession *> createCaptureSession() { return notAvailable; }
    virtual QMaybe<QPlatformMediaPlayer *> createPlayer(QMediaPlayer *) { return notAvailable; }
    virtual QMaybe<QPlatformMediaRecorder *> createRecorder(QMediaRecorder *) { return notAvailable; }
    virtual QMaybe<QPlatformImageCapture *> createImageCapture(QImageCapture *) { return notAvailable; }

    virtual QMaybe<QPlatformAudioInput *> createAudioInput(QAudioInput *);
    virtual QMaybe<QPlatformAudioOutput *> createAudioOutput(QAudioOutput *);

    virtual QMaybe<QPlatformVideoSink *> createVideoSink(QVideoSink *) { return notAvailable; }

    QList<QCapturableWindow> capturableWindows();
    bool isCapturableWindowValid(const QCapturableWindowPrivate &);

    QPlatformVideoDevices *videoDevices() { return m_videoDevices.get(); }

protected:
    virtual QPlatformMediaFormatInfo *createFormatInfo();

private:
    friend class QMockIntegrationFactory;
    // API to be able to test with a mock backend
    using Factory = std::function<std::unique_ptr<QPlatformMediaIntegration>()>;
    struct InstanceHolder;
    static void setPlatformFactory(Factory factory);

protected:
    std::unique_ptr<QPlatformVideoDevices> m_videoDevices;
    std::unique_ptr<QPlatformCapturableWindows> m_capturableWindows;

    mutable std::unique_ptr<QPlatformMediaFormatInfo> m_formatInfo;
    mutable std::once_flag m_formatInfoOnceFlg;
};

QT_END_NAMESPACE


#endif // QPLATFORMMEDIAINTERFACE_H
