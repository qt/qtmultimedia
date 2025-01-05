// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "recordingengine/qffmpegrecordingengineutils_p.h"
#include "recordingengine/qffmpegencoderthread_p.h"
#include "private/qplatformaudiobufferinput_p.h"
#include "private/qplatformvideoframeinput_p.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

template <typename F>
static void doWithMediaFrameInput(QObject *source, F &&f)
{
    if (auto videoFrameInput = qobject_cast<QPlatformVideoFrameInput *>(source))
        f(videoFrameInput);
    else if (auto audioBufferInput = qobject_cast<QPlatformAudioBufferInput *>(source))
        f(audioBufferInput);
}

void setEncoderInterface(QObject *source, QMediaInputEncoderInterface *encoderInterface)
{
    doWithMediaFrameInput(source, [&](auto source) {
        using Source = std::remove_pointer_t<decltype(source)>;

        source->setEncoderInterface(encoderInterface);
        if (encoderInterface)
            // Postpone emit 'encoderUpdated' as the encoding pipeline may be not
            // completely ready at the moment. The case is calling QMediaRecorder::stop
            // upon handling 'readyToSendFrame'
            QMetaObject::invokeMethod(source, &Source::encoderUpdated, Qt::QueuedConnection);
        else
            emit source->encoderUpdated();
    });
}

void setEncoderUpdateConnection(QObject *source, EncoderThread *encoder)
{
    doWithMediaFrameInput(source, [&](auto source) {
        using Source = std::remove_pointer_t<decltype(source)>;
        QObject::connect(encoder, &EncoderThread::canPushFrameChanged, source,
                         &Source::encoderUpdated);
    });
}

void disconnectEncoderFromSource(EncoderThread *encoder)
{
    QObject *source = encoder->source();
    if (!source)
        return;

    // We should address the dependency AudioEncoder from QFFmpegAudioInput to
    // set null source here.
    // encoder->setSource(nullptr);

    QObject::disconnect(source, nullptr, encoder, nullptr);
    setEncoderInterface(source, nullptr);
}

} // namespace QFFmpeg

QT_END_NAMESPACE
