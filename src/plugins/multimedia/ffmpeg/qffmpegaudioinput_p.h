// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGAUDIOINPUT_H
#define QFFMPEGAUDIOINPUT_H

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

#include <QtMultimedia/private/qplatformaudioinput_p.h>
#include <QtMultimedia/private/qplatformaudiobufferinput_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegthread_p.h>
#include <qaudioinput.h>

QT_BEGIN_NAMESPACE

class QAudioSource;
class QAudioBuffer;
namespace QFFmpeg {
class AudioSourceIO;
} // namespace QFFmpeg

constexpr int DefaultAudioInputBufferSize = 4096;

class QFFmpegAudioInput : public QAudioBufferSource, public QPlatformAudioInput
{
    // for qobject_cast
    Q_OBJECT
public:
    QFFmpegAudioInput(QAudioInput *qq);
    ~QFFmpegAudioInput() override;

    void setAudioDevice(const QAudioDevice &/*device*/) override;
    void setMuted(bool /*muted*/) override;
    void setVolume(float /*volume*/) override;

    void setBufferSize(int bufferSize);

    int bufferSize() const;

protected:
    // ensures the underlying audio source is run if
    // the signal newAudioBuffer is connected
    void connectNotify(const QMetaMethod &signal) override;

    // postponly ensures that the underlying audio source is not run
    // if the signal newAudioBuffer is not connacted to any slot.
    void disconnectNotify(const QMetaMethod &signal) override;

private:
    QFFmpeg::AudioSourceIO *m_audioIO = nullptr;
    std::unique_ptr<QThread> m_inputThread;
};

QT_END_NAMESPACE


#endif // QPLATFORMAUDIOINPUT_H
