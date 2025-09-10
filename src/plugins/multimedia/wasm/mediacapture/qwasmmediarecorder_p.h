// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMMEDIARECORDER_H
#define QWASMMEDIARECORDER_H

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

#include <private/qplatformmediarecorder_p.h>
#include <private/qplatformmediacapture_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qloggingcategory.h>
#include <QElapsedTimer>

#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>
#include <private/qstdweb_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qWasmMediaRecorder)

class QWasmMediaCaptureSession;
class QIODevice;

class QWasmMediaRecorder final : public QObject, public QPlatformMediaRecorder
{
    Q_OBJECT
public:
    explicit QWasmMediaRecorder(QMediaRecorder *parent);
    ~QWasmMediaRecorder() final;

    bool isLocationWritable(const QUrl &location) const override;
    QMediaRecorder::RecorderState state() const override;
    qint64 duration() const override;
    void record(QMediaEncoderSettings &settings) override;
    void pause() override;
    void resume() override;
    void stop() override;

    void setCaptureSession(QPlatformMediaCaptureSession *session);

private:

    bool hasCamera() const;
    void startAudioRecording();
    void setStream(emscripten::val stream);
    void streamCallback(emscripten::val event);
    void exceptionCallback(emscripten::val event);
    void dataAvailableCallback(emscripten::val dataEvent);
    void startStream();
    void setTrackContraints(QMediaEncoderSettings &settings, emscripten::val stream);
    void initUserMedia();
    void audioDataAvailable(emscripten::val Blob, double timeCodeDifference);
    void setUpFileSink();

    emscripten::val m_mediaRecorder = emscripten::val::undefined();
    emscripten::val m_mediaStream = emscripten::val::undefined();

    QWasmMediaCaptureSession *m_session = nullptr;
    QMediaEncoderSettings m_mediaSettings;
    QIODevice *m_outputTarget;
    QScopedPointer<qstdweb::EventCallback> m_mediaStreamDataAvailable;
    QScopedPointer<qstdweb::EventCallback> m_mediaStreamStopped;
    QScopedPointer<qstdweb::EventCallback> m_mediaStreamError;
    QScopedPointer<qstdweb::EventCallback> m_mediaStreamStart;
    QScopedPointer<qstdweb::EventCallback> m_mediaStreamPause;
    QScopedPointer<qstdweb::EventCallback> m_mediaStreamResume;

    qint64 m_durationMs = 0;
    bool m_isRecording = false;
    QScopedPointer <QElapsedTimer> m_durationTimer;

private Q_SLOTS:
};

QT_END_NAMESPACE

#endif // QWASMMEDIARECORDER_H
