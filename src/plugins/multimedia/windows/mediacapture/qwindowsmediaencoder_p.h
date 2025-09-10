// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#ifndef QWINDOWSMEDIAENCODER_H
#define QWINDOWSMEDIAENCODER_H

#include <private/qplatformmediarecorder_p.h>

#include <QtCore/qglobal.h>
#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

class QWindowsMediaDeviceSession;
class QPlatformMediaCaptureSession;
class QWindowsMediaCaptureService;

class QWindowsMediaEncoder : public QObject, public QPlatformMediaRecorder
{
    Q_OBJECT
public:
    explicit QWindowsMediaEncoder(QMediaRecorder *parent);

    bool isLocationWritable(const QUrl &location) const override;
    QMediaRecorder::RecorderState state() const override;
    qint64 duration() const override;

    void setMetaData(const QMediaMetaData &metaData) override;
    QMediaMetaData metaData() const override;

    void setCaptureSession(QPlatformMediaCaptureSession *session);

    void record(QMediaEncoderSettings &settings) override;
    void pause() override;
    void resume() override;
    void stop() override;

private Q_SLOTS:
    void onCameraChanged();
    void onRecordingStarted();
    void onRecordingStopped();
    void onDurationChanged(qint64 duration);
    void onStreamingError(int errorCode);
    void onRecordingError(int errorCode);

private:
    void saveMetadata();

    QWindowsMediaCaptureService  *m_captureService = nullptr;
    QWindowsMediaDeviceSession   *m_mediaDeviceSession = nullptr;
    QMediaRecorder::RecorderState          m_state = QMediaRecorder::StoppedState;
    QString                       m_fileName;
    QMediaMetaData                m_metaData;
    qint64                        m_duration = 0;
    bool                          m_sessionWasActive = false;
};

QT_END_NAMESPACE

#endif
