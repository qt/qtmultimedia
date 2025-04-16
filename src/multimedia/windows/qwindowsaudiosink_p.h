// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QWINDOWSAUDIOSINK_H
#define QWINDOWSAUDIOSINK_H

#include <QtMultimedia/private/qaudiosystem_p.h>

QT_BEGIN_NAMESPACE

struct QWASAPIAudioSinkStream;

class QWindowsAudioSink final : public QPlatformAudioSink
{
    Q_OBJECT
public:
    QWindowsAudioSink(QAudioDevice, const QAudioFormat &, QObject *parent);
    ~QWindowsAudioSink();

    QIODevice* start() override;
    void start(QIODevice* device) override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesFree() const override;
    void setBufferSize(qsizetype value) override;
    qsizetype bufferSize() const override;
    void setHardwareBufferFrames(int32_t) override;
    int32_t hardwareBufferFrames() override;
    qint64 processedUSecs() const override;
    void setVolume(float) override;

    void setRole(AudioEndpointRole) override;

    void start(AudioCallback &&) override;
    bool hasCallbackAPI() override;

private:
    friend QtMultimediaPrivate::QPlatformAudioSinkStream;
    friend struct QWASAPIAudioSinkStream;

    std::optional<qsizetype> m_bufferSize;
    std::optional<int32_t> m_hardwareBufferFrames;

    AudioEndpointRole m_endpointRole = AudioEndpointRole::Other;

    std::shared_ptr<QWASAPIAudioSinkStream> m_stream;
};

QT_END_NAMESPACE

#endif // QWINDOWSAUDIOSINK_H
