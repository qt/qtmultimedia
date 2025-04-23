// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMJS_P_H
#define QWASMJS_P_H

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

#include <QObject>

#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>
#include <private/qstdweb_p.h>
#include <private/qplatformmediarecorder_p.h>

QT_BEGIN_NAMESPACE

class QIODevice;

class JsMediaRecorder final : public QIODevice
{
    Q_OBJECT
public:
    JsMediaRecorder();
    JsMediaRecorder(const QIODevice *outputDevice);

    void pauseStream();
    void resumeStream();
    void stopStream();
    void startStreaming();

    bool open(QIODeviceBase::OpenMode mode) override;
    bool isSequential() const override;
    qint64 size() const override;
    bool seek(qint64 pos) override;
    void setStream(emscripten::val stream);
    qint64 bytesAvailable() const override;

    void setNeedsCamera(bool hasCamera) { m_needsCamera = hasCamera; }
    void setNeedsAudio(bool hasAudio) { m_needsAudio = hasAudio; }

    QMediaRecorder::RecorderState currentState() { return m_currentState; }

Q_SIGNALS:
    void started();
    void stopped();
    void paused();
    void resumed();
    void streamError(QMediaRecorder::Error error, const QString &errorMessage);
    void stateChanged(QMediaRecorder::RecorderState state);

protected:
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *, qint64) override;

private:
    void audioDataAvailable(emscripten::val Blob, double timeCodeDifference);
    void setTrackContraints(QMediaEncoderSettings &settings, emscripten::val stream);

    emscripten::val m_mediaRecorder = emscripten::val::undefined();
    emscripten::val m_mediaStream = emscripten::val::undefined();
    QMediaEncoderSettings m_mediaSettings;
    bool m_needsCamera = false;
    bool m_needsAudio = false;

    QMediaRecorder::RecorderState m_currentState = QMediaRecorder::StoppedState;
    QByteArray m_buffer;

    QScopedPointer<qstdweb::EventCallback> m_mediaStreamDataAvailable;
    QScopedPointer<qstdweb::EventCallback> m_mediaStreamStopped;
    QScopedPointer<qstdweb::EventCallback> m_mediaStreamError;
    QScopedPointer<qstdweb::EventCallback> m_mediaStreamStart;
    QScopedPointer<qstdweb::EventCallback> m_mediaStreamPause;
    QScopedPointer<qstdweb::EventCallback> m_mediaStreamResume;
};

class JsMediaInputStream : public QObject
{
    Q_OBJECT
public:
    explicit JsMediaInputStream(QObject *parent = nullptr);

    bool isActive() { return m_active; }

    void setUseAudio(bool useAudio) { m_needsAudio = useAudio; }
    void setUseVideo(bool useVideo) { m_needsVideo = useVideo; }
    void setStreamDevice(const std::string &id);
    emscripten::val getMediaStream() { return m_mediaStream; }

signals:
    void mediaStreamReady();
    void activated(bool active);

private:
    void setupMediaStream(emscripten::val mStream);
    emscripten::val m_mediaStream = emscripten::val::undefined();
    bool m_needsAudio = false;
    bool m_needsVideo = false;
    bool m_active = false;

    QScopedPointer<qstdweb::EventCallback> m_activeStreamEvent;
    QScopedPointer<qstdweb::EventCallback> m_inactiveStreamEvent;

};

#endif // QWASMJS_P_H
