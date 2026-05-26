// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSCAMERASESSION_P_H
#define QOHOSCAMERASESSION_P_H

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

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/qurl.h>
#include <QtMultimedia/qcameradevice.h>
#include <QtMultimedia/qimagecapture.h>
#include <QtMultimedia/qmediarecorder.h>
#include <QtMultimedia/qvideosink.h>

#include <private/qplatformimagecapture_p.h>
#include <private/qplatformmediarecorder_p.h>

#include <ohcamera/camera.h>
#include <ohcamera/camera_input.h>
#include <ohcamera/camera_manager.h>
#include <ohcamera/capture_session.h>
#include <ohcamera/photo_output.h>
#include <ohcamera/preview_output.h>
#include <ohcamera/video_output.h>

#include <multimedia/image_framework/image/image_receiver_native.h>
#include <multimedia/player_framework/avrecorder.h>
#include <multimedia/player_framework/avrecorder_base.h>

#include <atomic>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

class QOhosVideoOutput;

class QOhosCameraSession : public QObject
{
    Q_OBJECT
public:
    explicit QOhosCameraSession(QObject *parent = nullptr);
    ~QOhosCameraSession() override;

    void setCamera(const QCameraDevice &camera);
    QCameraDevice camera() const { return m_cameraDevice; }

    void setCameraFormat(const QCameraFormat &format);
    QCameraFormat cameraFormat() const { return m_cameraFormat; }

    void setVideoSink(QVideoSink *sink);
    QVideoSink *videoSink() const { return m_videoSink; }

    void setActive(bool active);
    bool isActive() const { return m_active; }

    bool isReadyForCapture() const { return m_active && m_photoOutput && !m_captureInProgress; }
    int capture(const QString &fileName, bool toBuffer = false);

    QImageEncoderSettings imageSettings() const { return m_imageSettings; }
    void setImageSettings(const QImageEncoderSettings &settings);

    QMediaRecorder::RecorderState recorderState() const { return m_recorderState; }
    qint64 recorderDuration() const;
    bool startRecording(const QMediaEncoderSettings &settings, const QString &location);
    void pauseRecording();
    void resumeRecording();
    void stopRecording();

signals:
    void activeChanged(bool active);
    void errorOccurred(int code, const QString &message);
    void readyForCaptureChanged(bool ready);
    void imageExposed(int id);
    void imageCaptured(int id, const QImage &preview);
    void imageAvailable(int id, const QVideoFrame &frame);
    void imageSaved(int id, const QString &fileName);
    void imageCaptureError(int id, int error, const QString &message);

    void recorderStateChanged(int state);
    void recorderErrorOccurred(int code, const QString &message);
    void recorderDurationChanged(qint64 ms);
    void recorderActualLocationChanged(const QUrl &url);

public slots:
    void onCapturedImageAvailable();
    void onRecorderStateNotification(int state);
    void onRecorderErrorNotification(int code, const QString &message);

private slots:
    void onSurfaceReady();

private:
    void emitReadyForCaptureChanged();

    bool startSession();
    void stopSession();
    void releaseSession();
    bool ensureManager();
    Camera_Device *findDevice(const QByteArray &id);
    bool createPhotoPath(Camera_OutputCapability *caps, Camera_Profile *previewProfile);
    void destroyPhotoPath();
    bool attachVideoOutput(const Camera_VideoProfile &profile, const QByteArray &surfaceId);
    void detachVideoOutput();
    void destroyRecorder();
    bool findVideoProfile(const QMediaEncoderSettings &settings, Camera_VideoProfile *out);

    static void recorderStateCallback(OH_AVRecorder *recorder, OH_AVRecorder_State state,
                                      OH_AVRecorder_StateChangeReason reason, void *userData);
    static void recorderErrorCallback(OH_AVRecorder *recorder, int32_t errorCode,
                                      const char *errorMsg, void *userData);

    Camera_Manager *m_manager{ nullptr };
    Camera_Device *m_supportedDevices{ nullptr };
    uint32_t m_supportedDeviceCount{ 0 };

    Camera_Input *m_cameraInput{ nullptr };
    Camera_CaptureSession *m_captureSession{ nullptr };
    Camera_PreviewOutput *m_previewOutput{ nullptr };
    Camera_PhotoOutput *m_photoOutput{ nullptr };
    Camera_VideoOutput *m_videoOutputCamera{ nullptr };

    OH_ImageReceiverNative *m_imageReceiver{ nullptr };
    OH_ImageReceiverOptions *m_imageReceiverOptions{ nullptr };

    OH_AVRecorder *m_recorder{ nullptr };
    OHNativeWindow *m_recorderWindow{ nullptr };
    QMediaRecorder::RecorderState m_recorderState{ QMediaRecorder::StoppedState };
    QUrl m_recorderActualLocation;
    QElapsedTimer m_recorderTimer;
    qint64 m_recorderPausedMs{ 0 };
    qint64 m_recorderResumeStartMs{ 0 };

    QCameraDevice m_cameraDevice;
    QCameraFormat m_cameraFormat;
    QPointer<QVideoSink> m_videoSink;
    std::unique_ptr<QOhosVideoOutput> m_videoOutput;

    QImageEncoderSettings m_imageSettings;
    std::atomic<int> m_lastCaptureId{ 0 };
    int m_pendingCaptureId{ 0 };
    QString m_pendingCaptureFileName;
    bool m_pendingCaptureToBuffer{ false };
    bool m_captureInProgress{ false };

    bool m_active{ false };
    bool m_pendingStart{ false };
    std::optional<bool> m_lastReadyForCapture;
};

QT_END_NAMESPACE

#endif // QOHOSCAMERASESSION_P_H
