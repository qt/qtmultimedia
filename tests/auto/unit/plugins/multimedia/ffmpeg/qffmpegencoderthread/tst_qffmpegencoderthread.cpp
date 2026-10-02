// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtCore/qdeadlinetimer.h>
#include <QtCore/qthread.h>

#include <QtFFmpegMediaPluginImpl/private/qffmpegencoderthread_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegrecordingengine_p.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <new>
#include <thread>

using namespace QFFmpeg;
using namespace std::chrono_literals;

namespace {

class IdleEncoder : public EncoderThread
{
public:
    using EncoderThread::EncoderThread;
    using EncoderThread::setEndOfSourceStream;

protected:
    bool checkIfCanPushFrame() const override { return true; }
    bool init() override { return true; }
    void cleanup() override { }
    bool hasData() const override { return false; }
    void processOne() override { }
};

// A real RecordingEngine asserts a completed finalization on destruction. The encoders here are
// never started, so the reference is never dereferenced.
RecordingEngine &unusedRecordingEngine()
{
    alignas(RecordingEngine) static std::byte storage[sizeof(RecordingEngine)];
    return *reinterpret_cast<RecordingEngine *>(storage);
}

} // namespace

class tst_QFFmpegEncoderThread : public QObject
{
    Q_OBJECT

private slots:
    void isEndOfSourceStream_isFalse_byDefault();
    void isEndOfSourceStream_isVisibleToOtherThread_whenSetFromWorkerThread();
};

void tst_QFFmpegEncoderThread::isEndOfSourceStream_isFalse_byDefault()
{
    ConsumerThreadUPtr<IdleEncoder> encoder(new IdleEncoder(unusedRecordingEngine()));
    QVERIFY(!encoder->isEndOfSourceStream());
}

void tst_QFFmpegEncoderThread::
        isEndOfSourceStream_isVisibleToOtherThread_whenSetFromWorkerThread()
{
    constexpr int iterations = 500;

    for (int i = 0; i < iterations; ++i) {
        ConsumerThreadUPtr<IdleEncoder> audio(new IdleEncoder(unusedRecordingEngine()));
        ConsumerThreadUPtr<IdleEncoder> video(new IdleEncoder(unusedRecordingEngine()));

        std::atomic_bool go = false;
        auto reportEos = [&](IdleEncoder *encoder) {
            while (!go.load())
                std::this_thread::yield();
            encoder->setEndOfSourceStream();
        };

        std::thread audioSource(reportEos, audio.get());
        std::thread videoSource(reportEos, video.get());

        go = true;

        QDeadlineTimer deadline(5s);
        bool allEnded = false;
        while (!deadline.hasExpired()) {
            allEnded = audio->isEndOfSourceStream() && video->isEndOfSourceStream();
            if (allEnded)
                break;
            std::this_thread::yield();
        }

        audioSource.join();
        videoSource.join();

        QVERIFY2(allEnded, "End of source stream was never observed by the polling thread");
    }
}

QTEST_GUILESS_MAIN(tst_QFFmpegEncoderThread)

#include "tst_qffmpegencoderthread.moc"
