// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGMEDIACAPTURESESSION_H
#define QFFMPEGMEDIACAPTURESESSION_H

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

#include <QtCore/qiodevice.h>
#include <QtCore/qpointer.h>

#include <QtMultimedia/private/qplatformmediacapture_p.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>

QT_BEGIN_NAMESPACE

class QFFmpegMediaRecorder;
class QFFmpegImageCapture;
class QVideoFrame;
class QAudioSink;
class QFFmpegAudioInput;
class QAudioBuffer;
class QPlatformVideoSource;
class QPlatformAudioBufferInput;
class QAudioBufferSource;

class QFFmpegMediaCaptureSession : public QPlatformMediaCaptureSession
{
    Q_OBJECT

public:
    using VideoSources = std::vector<QPointer<QPlatformVideoSource>>;

    QFFmpegMediaCaptureSession();
    ~QFFmpegMediaCaptureSession() override;

    QPlatformCamera *camera() override;
    void setCamera(QPlatformCamera *camera) override;

    QPlatformSurfaceCapture *screenCapture() override;
    void setScreenCapture(QPlatformSurfaceCapture *) override;

    QPlatformSurfaceCapture *windowCapture() override;
    void setWindowCapture(QPlatformSurfaceCapture *) override;

    QPlatformVideoFrameInput *videoFrameInput() override;
    void setVideoFrameInput(QPlatformVideoFrameInput *) override;

    QPlatformImageCapture *imageCapture() override;
    void setImageCapture(QPlatformImageCapture *imageCapture) override;

    QPlatformMediaRecorder *mediaRecorder() override;
    void setMediaRecorder(QPlatformMediaRecorder *recorder) override;

    void setAudioInput(QPlatformAudioInput *input) override;
    QPlatformAudioInput *audioInput() const;

    void setAudioBufferInput(QPlatformAudioBufferInput *input) override;

    void setVideoPreview(QVideoSink *sink) override;
    void setAudioOutput(QPlatformAudioOutput *output) override;

    // A source may emit frames from a backgrund thread before
    // its setActive() is done. We need to setup necessary connections
    // ahead of time as to not miss initial frames. This function
    // should be called by a source right before it initiates
    // the internal stream.
    void onSourceActivating(QPlatformVideoSource &);
    // Called by a source if it fails to go inactive.
    void onSourceActivationFailure(QPlatformVideoSource &);

    QPlatformVideoSource *primaryActiveVideoSource();

    // it might be moved to the base class, but it needs QPlatformAudioInput
    // to be QAudioBufferSource, which might not make sense
    std::vector<QAudioBufferSource *> activeAudioInputs() const;

private Q_SLOTS:
    void updateAudioSink();
    void updateVolume();
    void updatePrimaryActiveVideoSource();

Q_SIGNALS:
    void primaryActiveVideoSourceChanged();

private:
    template<typename VideoSource>
    bool setVideoSource(QPointer<VideoSource> &source, VideoSource *newSource);

    [[nodiscard]] QPlatformVideoSource *primaryVideoSource(QPlatformVideoSource *pendingActiveSource);

    void updateVideoFrameConnection(QPlatformVideoSource *pendingActiveSource);

    QPointer<QPlatformCamera> m_camera;
    QPointer<QPlatformSurfaceCapture> m_screenCapture;
    QPointer<QPlatformSurfaceCapture> m_windowCapture;
    QPointer<QPlatformVideoFrameInput> m_videoFrameInput;
    QPointer<QPlatformVideoSource> m_primaryActiveVideoSource;

    QPointer<QFFmpegAudioInput> m_audioInput;
    QPointer<QPlatformAudioBufferInput> m_audioBufferInput;

    QFFmpegImageCapture *m_imageCapture = nullptr;
    QFFmpegMediaRecorder *m_mediaRecorder = nullptr;
    QPlatformAudioOutput *m_audioOutput = nullptr;
    QVideoSink *m_videoSink = nullptr;
    std::unique_ptr<QAudioSink> m_audioSink;
    QPointer<QIODevice> m_audioIODevice;
    qsizetype m_audioBufferSize = 0;

    struct SourceSinkConnection {
        QPlatformVideoSource *source = nullptr;
        QVideoSink *sink = nullptr;
        QMetaObject::Connection connection;
    };

    SourceSinkConnection m_videoSinkConnection;
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAPTURESERVICE_H
