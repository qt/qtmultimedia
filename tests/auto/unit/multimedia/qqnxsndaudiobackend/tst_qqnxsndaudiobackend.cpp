// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/qaudiosink.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/private/qaudiodevice_p.h>
#include <QtMultimedia/private/qqnxsndaudiodevice_p.h>

#include <QtCore/qbuffer.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qthread.h>

#include <set>

QT_USE_NAMESPACE

// io-snd-specific backend tests. They QSKIP cleanly when no output device
// is enumerated (host build, or QNX target without audio HW), so the test
// is safe to run anywhere — only meaningful coverage requires the QNX
// target with /dev/snd populated.

class tst_QQnxSndAudioBackend : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void enumerateOutputs_noDuplicates_oneDefault();
    void openInvalidDeviceId_failsCleanly();
    void stopWhileSuspended_destroysCleanly();
    void pushUnderrun_recoversCleanly();

private:
    QAudioDevice m_defaultOutput;
};

void tst_QQnxSndAudioBackend::initTestCase()
{
    const auto outputs = QMediaDevices::audioOutputs();
    if (outputs.isEmpty())
        QSKIP("No audio output devices enumerated — io-snd backend not available");
    m_defaultOutput = QMediaDevices::defaultAudioOutput();
    QVERIFY(!m_defaultOutput.isNull());
}

// W1.2 — availableDevicesViaHints must not double-insert the fallback default.
void tst_QQnxSndAudioBackend::enumerateOutputs_noDuplicates_oneDefault()
{
    const auto outputs = QMediaDevices::audioOutputs();
    QVERIFY(!outputs.isEmpty());

    std::set<QByteArray> ids;
    for (const auto &d : outputs) {
        QVERIFY2(ids.insert(d.id()).second,
                 ("duplicate audio output id: " + d.id()).constData());
    }

    int defaultCount = 0;
    for (const auto &d : outputs) {
        if (d.isDefault())
            ++defaultCount;
    }
    QCOMPARE(defaultCount, 1);
}

// Error path: opening a sink against a fabricated device id must surface as
// an error and not leak the worker.
void tst_QQnxSndAudioBackend::openInvalidDeviceId_failsCleanly()
{
    auto info = std::make_unique<QQnxSndAudioDeviceInfo>(
            QByteArray("hw:bogus_doesnotexist_99"),
            QStringLiteral("Bogus"),
            QAudioDevice::Output);
    const QAudioDevice bogus = QAudioDevicePrivate::createQAudioDevice(std::move(info));

    QAudioSink sink(bogus, m_defaultOutput.preferredFormat());

    QBuffer buffer;
    QByteArray data(4096, '\0');
    buffer.setData(data);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    sink.start(&buffer);

    // Within 1 s the sink must either report an error or have transitioned
    // to StoppedState. A successful open against a non-existent device
    // would be a failure of openPcmDevice's retry/return-checking.
    QTRY_VERIFY_WITH_TIMEOUT(
            sink.error() != QAudio::NoError || sink.state() == QAudio::StoppedState,
            1000);
    QVERIFY(sink.state() != QAudio::ActiveState);
}

// W1.4 — stop(DrainRingbuffer) on a suspended stream must not hang.
void tst_QQnxSndAudioBackend::stopWhileSuspended_destroysCleanly()
{
    const QAudioFormat fmt = m_defaultOutput.preferredFormat();

    QAudioSink sink(m_defaultOutput, fmt);
    QIODevice *push = sink.start();
    QVERIFY(push != nullptr);

    // Feed enough silence to enter ActiveState.
    const QByteArray silence(fmt.bytesForDuration(50'000), 0);
    push->write(silence);
    QTRY_COMPARE_WITH_TIMEOUT(sink.state(), QAudio::ActiveState, 1000);

    sink.suspend();
    QCOMPARE(sink.state(), QAudio::SuspendedState);

    QElapsedTimer timer;
    timer.start();
    sink.stop();
    QVERIFY2(timer.elapsed() < 1000,
             "sink.stop() while suspended took >1s — drain leak likely");
    QCOMPARE(sink.state(), QAudio::StoppedState);
}

// W1.3 — running the hardware dry then feeding again must not surface as a
// fatal error. Underrun recovery succeeds and playback resumes.
void tst_QQnxSndAudioBackend::pushUnderrun_recoversCleanly()
{
    const QAudioFormat fmt = m_defaultOutput.preferredFormat();

    QAudioSink sink(m_defaultOutput, fmt);
    QIODevice *push = sink.start();
    QVERIFY(push != nullptr);

    const QByteArray chunk(fmt.bytesForDuration(80'000), 0);
    push->write(chunk);
    QTRY_COMPARE_WITH_TIMEOUT(sink.state(), QAudio::ActiveState, 1000);

    // Let the hardware drain past the configured buffer time so the worker
    // hits an underrun and runs through recoverFromXrun.
    QThread::msleep(400);

    push->write(chunk);
    QTRY_VERIFY_WITH_TIMEOUT(
            sink.state() == QAudio::ActiveState || sink.state() == QAudio::IdleState,
            1000);
    QCOMPARE(sink.error(), QAudio::NoError);
    sink.stop();
}

QTEST_MAIN(tst_QQnxSndAudioBackend)

#include "tst_qqnxsndaudiobackend.moc"
