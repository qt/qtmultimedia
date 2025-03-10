// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MEDIAFRAMEINPUTQUEUE_H
#define MEDIAFRAMEINPUTQUEUE_H

#include <QVideoFrame>
#include <QAudioBuffer>
#include <QAudioBufferInput>
#include <QVideoFrameInput>
#include <QDebug>

#include <queue>

struct AudioBufferInputQueueTraits
{
    using MediaFrame = QAudioBuffer;
    using MediaFrameInput = QAudioBufferInput;
    static inline const auto sendMediaFrame = &QAudioBufferInput::sendAudioBuffer;
    static inline const auto readyToSend = &QAudioBufferInput::readyToSendAudioBuffer;
    static constexpr char name[] = "audio";
};

struct VideoFrameInputQueueTraits
{
    using MediaFrame = QVideoFrame;
    using MediaFrameInput = QVideoFrameInput;
    static inline const auto sendMediaFrame = &QVideoFrameInput::sendVideoFrame;
    static inline const auto readyToSend = &QVideoFrameInput::readyToSendVideoFrame;
    static constexpr char name[] = "video";
};

template <typename Traits>
class MediaFrameInputQueue
{
public:
    MediaFrameInputQueue(size_t maxQueueSize = 5) : m_maxQueueSize(maxQueueSize)
    {
        QObject::connect(&m_input, Traits::readyToSend, &m_input, [this]() {
            while (!m_queue.empty() && (m_input.*Traits::sendMediaFrame)(m_queue.front()))
                m_queue.pop();
        });
    }

    auto *mediaFrameInput() { return &m_input; }

    void pushMediaFrame(const typename Traits::MediaFrame &mediaFrame)
    {
        const bool sent = m_queue.empty() && (m_input.*Traits::sendMediaFrame)(mediaFrame);
        if (!sent) {
            if (m_queue.size() < m_maxQueueSize)
                m_queue.push(mediaFrame);
            else
                qWarning() << "Drop" << Traits::name << "frame with startTime"
                           << mediaFrame.startTime() / 1000 << "ms";
        }
    }

private:
    typename Traits::MediaFrameInput m_input;
    std::queue<typename Traits::MediaFrame> m_queue;
    size_t m_maxQueueSize;
};

#endif // MEDIAFRAMEINPUTQUEUE_H
