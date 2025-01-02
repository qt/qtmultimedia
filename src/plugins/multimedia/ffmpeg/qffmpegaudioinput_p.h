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

#include <private/qplatformaudioinput_p.h>
#include "qffmpegthread_p.h"
#include <qaudioinput.h>

QT_BEGIN_NAMESPACE

class QAudioSource;
class QAudioBuffer;
namespace QFFmpeg {
class AudioSourceIO;
}

constexpr int DefaultAudioInputBufferSize = 4096;

class QFFmpegAudioInput : public QObject, public QPlatformAudioInput
{
    Q_OBJECT
public:
    QFFmpegAudioInput(QAudioInput *qq);
    ~QFFmpegAudioInput();

    void setAudioDevice(const QAudioDevice &/*device*/) override;
    void setMuted(bool /*muted*/) override;
    void setVolume(float /*volume*/) override;

    void setFrameSize(int frameSize);
    void setRunning(bool b);

    int bufferSize() const;

Q_SIGNALS:
    void newAudioBuffer(const QAudioBuffer &buffer);

private:
    std::unique_ptr<QFFmpeg::AudioSourceIO> audioIO;
    std::unique_ptr<QThread> inputThread;
};

QT_END_NAMESPACE


#endif // QPLATFORMAUDIOINPUT_H
