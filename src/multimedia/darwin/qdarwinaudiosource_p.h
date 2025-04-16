// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef IOSAUDIOINPUT_H
#define IOSAUDIOINPUT_H

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
#include <QtCore/qtimer.h>

#include <QtMultimedia/private/qaudioringbuffer_p.h>
#include <QtMultimedia/private/qaudiostatemachine_p.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qcoreaudioutils_p.h>
#include <QtMultimedia/private/qdarwinaudiodevice_p.h>

#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudioTypes.h>
#include <AudioToolbox/AudioConverter.h>


QT_BEGIN_NAMESPACE

class QCoreAudioSourceStream;

class QDarwinAudioSource final : public QPlatformAudioSource
{
public:
    QDarwinAudioSource(QAudioDevice, const QAudioFormat &, QObject *parent);
    ~QDarwinAudioSource() override;

    void start(QIODevice *device) override;
    QIODevice *start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesReady() const override;
    void setBufferSize(qsizetype value) override;
    qsizetype bufferSize() const override;
    void setHardwareBufferFrames(int32_t) override;
    int32_t hardwareBufferFrames() override;
    qint64 processedUSecs() const override;

    void setVolume(float volume) override;

private:
    friend class QCoreAudioSourceStream;
    friend class QtMultimediaPrivate::QPlatformAudioSourceStream;

    std::shared_ptr<QCoreAudioSourceStream> m_stream;
    std::optional<int> m_internalBufferSize;
    std::optional<int32_t> m_hardwareBufferFrames;

    void resumeStreamIfNecessary();

    std::shared_ptr<QCoreAudioSourceStream> m_retiredStream;
};

QT_END_NAMESPACE

#endif // IOSAUDIOINPUT_H
