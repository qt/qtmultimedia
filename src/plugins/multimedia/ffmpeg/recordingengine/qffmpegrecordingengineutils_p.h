// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGRECORDINGENGINEUTILS_P_H
#define QFFMPEGRECORDINGENGINEUTILS_P_H

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

#include "qobject.h"
#include <queue>

QT_BEGIN_NAMESPACE

class QMediaInputEncoderInterface;
class QPlatformVideoSource;

namespace QFFmpeg {

class EncoderThread;

template <typename T>
T dequeueIfPossible(std::queue<T> &queue)
{
    if (queue.empty())
        return T{};

    auto result = std::move(queue.front());
    queue.pop();
    return result;
}

void setEncoderInterface(QObject *source, QMediaInputEncoderInterface *interface);

void setEncoderUpdateConnection(QObject *source, EncoderThread *encoder);

template <typename Encoder, typename Source>
void connectEncoderToSource(Encoder *encoder, Source *source)
{
    Q_ASSERT(!encoder->source());
    encoder->setSource(source);

    if constexpr (std::is_same_v<Source, QPlatformVideoSource>)
        QObject::connect(source, &Source::newVideoFrame, encoder, &Encoder::addFrame,
                         Qt::DirectConnection);
    else
        QObject::connect(source, &Source::newAudioBuffer, encoder, &Encoder::addBuffer,
                         Qt::DirectConnection);

    setEncoderUpdateConnection(source, encoder);
    setEncoderInterface(source, encoder);
}

void disconnectEncoderFromSource(EncoderThread *encoder);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGRECORDINGENGINEUTILS_P_H
