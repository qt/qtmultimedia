// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSMEDIARECORDER_P_H
#define QOHOSMEDIARECORDER_P_H

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
#include <QtCore/qpointer.h>
#include <QtCore/qtimer.h>
#include <QtCore/qurl.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/qaudiosource.h>
#include <QtMultimedia/qmediametadata.h>
#include <QtMultimedia/qmediarecorder.h>

#include <private/qplatformmediacapture_p.h>
#include <private/qplatformmediarecorder_p.h>

#include <multimedia/player_framework/avrecorder.h>
#include <multimedia/player_framework/avrecorder_base.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QFile;
class QOhosCameraSession;
class QOhosMediaCaptureSession;

class QOhosMediaRecorder : public QObject, public QPlatformMediaRecorder
{
    Q_OBJECT
public:
    explicit QOhosMediaRecorder(QMediaRecorder *parent);
    ~QOhosMediaRecorder() override;

    bool isLocationWritable(const QUrl &location) const override;

    QMediaRecorder::RecorderState state() const override;
    qint64 duration() const override;

    void record(QMediaEncoderSettings &settings) override;
    void pause() override;
    void resume() override;
    void stop() override;

    void setMetaData(const QMediaMetaData &metaData) override;
    QMediaMetaData metaData() const override { return m_metaData; }

    void setCaptureSession(QPlatformMediaCaptureSession *session);

private:
    void onRecorderStateChanged(int state);
    void onRecorderError(int code, const QString &message);
    void onRecorderDuration(qint64 ms);
    void onRecorderActualLocation(const QUrl &url);

    void onAudioOnlyStateNotification(int state);
    void onAudioOnlyErrorNotification(int code, const QString &message);

    void connectToSession();
    void disconnectFromSession();

    bool recordAudioOnly(const QMediaEncoderSettings &settings, const QString &location);
    void stopAudioOnly();
    void pauseAudioOnly();
    void resumeAudioOnly();
    void destroyAudioOnlyRecorder();

    // Wave/PCM recording bypasses OH_AVRecorder (which has no PCM codec) and
    // writes a RIFF/WAVE container from raw QAudioSource frames.
    bool recordWave(const QMediaEncoderSettings &settings, const QString &location);
    void stopWave();
    void pauseWave();
    void resumeWave();
    void destroyWaveRecorder();

    static void audioOnlyStateCallback(OH_AVRecorder *recorder, OH_AVRecorder_State state,
                                       OH_AVRecorder_StateChangeReason reason, void *userData);
    static void audioOnlyErrorCallback(OH_AVRecorder *recorder, int32_t errorCode,
                                       const char *errorMsg, void *userData);

    QPointer<QOhosMediaCaptureSession> m_service;
    QPointer<QOhosCameraSession> m_session;

    QMediaMetaData m_metaData;

    OH_AVRecorder *m_audioOnlyRecorder{ nullptr };
    int m_audioOnlyFd{ -1 };
    QMediaRecorder::RecorderState m_audioOnlyState{ QMediaRecorder::StoppedState };
    QUrl m_audioOnlyActualLocation;
    QElapsedTimer m_audioOnlyTimer;
    qint64 m_audioOnlyPausedMs{ 0 };
    qint64 m_audioOnlyResumeStartMs{ 0 };
    QTimer m_audioOnlyDurationTimer;

    // Wave/PCM path
    std::unique_ptr<QAudioSource> m_waveSource;
    std::unique_ptr<QFile> m_waveFile;
    QMediaRecorder::RecorderState m_waveState{ QMediaRecorder::StoppedState };
    QUrl m_waveActualLocation;
    QAudioFormat m_waveFormat;
    QElapsedTimer m_waveTimer;
    qint64 m_wavePausedMs{ 0 };
    qint64 m_waveResumeStartMs{ 0 };
};

QT_END_NAMESPACE

#endif // QOHOSMEDIARECORDER_P_H
