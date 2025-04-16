// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef IOSAUDIOOUTPUT_H
#define IOSAUDIOOUTPUT_H

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

#if defined(Q_OS_MACOS)
#  include <CoreAudio/CoreAudio.h>
#endif
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudioTypes.h>

#include <QtCore/qiodevice.h>
#include <QtCore/qsemaphore.h>
#include <QtMultimedia/private/qaudioringbuffer_p.h>
#include <QtMultimedia/private/qaudiostatemachine_p.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qcoreaudioutils_p.h>
#include <QtMultimedia/private/qdarwinaudiodevice_p.h>

QT_BEGIN_NAMESPACE

class QCoreAudioSinkStream;

class QDarwinAudioSink final : public QPlatformAudioSink
{
public:
    QDarwinAudioSink(QAudioDevice device, const QAudioFormat &format, QObject *parent);
    ~QDarwinAudioSink() override;

    void start(QIODevice *device) override;
    QIODevice *start() override;
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

    void setVolume(float volume) override;

    void start(AudioCallback &&) override;
    bool hasCallbackAPI() override;

private:
    void resumeStreamIfNecessary();

    friend class QtMultimediaPrivate::QPlatformAudioSinkStream;
    friend class QCoreAudioSinkStream;

    std::optional<int> m_internalBufferSize;
    std::optional<int32_t> m_hardwareBufferFrames;

    std::shared_ptr<QCoreAudioSinkStream> m_stream;
};

QT_END_NAMESPACE

#endif // IOSAUDIOOUTPUT_H
