// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtMultimedia/qmediarecorder.h>
#include <QtMultimedia/qmediacapturesession.h>
#include <QtMultimedia/qaudiobufferinput.h>
#include <QtMultimedia/qmediaformat.h>
#include <private/audiogenerationutils_p.h>
#include <private/mediabackendutils_p.h>
#include <private/capturesessionfixture_p.h>
#include <private/mediainfo_p.h>
#include <private/qcolorutil_p.h>
#include <private/qfileutil_p.h>

#include <QtCore/qtemporarydir.h>
#include <chrono>

using namespace std::chrono_literals;

QT_USE_NAMESPACE

namespace {
bool isSupportedPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat)
{
    // TODO: Enable more pixel formats once support is added
    switch (pixelFormat) {
    case QVideoFrameFormat::Format_AYUV:
    case QVideoFrameFormat::Format_AYUV_Premultiplied:
    case QVideoFrameFormat::Format_YV12:
    case QVideoFrameFormat::Format_IMC1:
    case QVideoFrameFormat::Format_IMC2:
    case QVideoFrameFormat::Format_IMC3:
    case QVideoFrameFormat::Format_IMC4:
    case QVideoFrameFormat::Format_YUV420P10: // TODO: Cpu conversion not implemented, fails in
                                              // CI if RHI is not supported
    case QVideoFrameFormat::Format_Y16: // TODO: Fails on Android
    case QVideoFrameFormat::Format_P010: // TODO: Fails on Android
    case QVideoFrameFormat::Format_P016: // TODO: Fails on Android
    case QVideoFrameFormat::Format_SamplerExternalOES:
    case QVideoFrameFormat::Format_Jpeg:
    case QVideoFrameFormat::Format_SamplerRect:
        return false;
    default:
        return true;
    }
}
} // namespace

using namespace Qt::StringLiterals;

class tst_qmediarecorderbackend : public QObject
{
    Q_OBJECT

public slots:

    void cleanupTestCase()
    {
        // Copy any stored files in the temporary directory over to COIN result directory
        // to allow inspecting image differences.
        if (qEnvironmentVariableIsSet("COIN_CTEST_RESULTSDIR")) {
            const QDir sourceDir = m_tempDir.path();
            const QDir resultsDir{ qEnvironmentVariable("COIN_CTEST_RESULTSDIR") };
            if (!copyAllFiles(sourceDir, resultsDir))
                qWarning() << "Failed to copy files to COIN_CTEST_RESULTSDIR";
        }
    }

private slots:
    void record_createsFileWithExpectedExtension_whenRecordingAudio_data()
    {
        QTest::addColumn<QMediaFormat::FileFormat>("fileFormat");
        QTest::addColumn<QString>("inputFileName");
        QTest::addColumn<QString>("expectedFileName");

        QMediaFormat format;
        for (const QMediaFormat::FileFormat &fileFormat :
             format.supportedFileFormats(QMediaFormat::Encode)) {

            const QByteArray formatName = QMediaFormat::fileFormatName(fileFormat).toLatin1();

            {
                // Verify that extension is appended if not already present
                QByteArray testName = formatName + " without extension";
                QString inputFileName = u"filename"_s;
                QString expected = inputFileName;

                const QMediaFormat mediaFormat(fileFormat);
                const QMimeType mimeType = mediaFormat.mimeType();
                const QString preferredExt = mimeType.preferredSuffix();
                if (!preferredExt.isEmpty())
                    expected += "." + preferredExt;

                QTest::addRow("%s", testName.data()) << fileFormat << inputFileName << expected;
            }

            {
                // Verify that default extension is not appended when extension is wrong
                QByteArray testName = formatName + " with wrong extension";
                QString inputFileName = u"filename.mp4"_s;
                QString expected = u"filename.mp4"_s;

                QTest::addRow("%s", testName.data()) << fileFormat << inputFileName << expected;
            }
        }
    }

    void record_createsFileWithExpectedExtension_whenRecordingAudio()
    {
        if (!isFFMPEGPlatform())
            QSKIP("This test requires APIs that are only implemented with FFmpeg media backend");

        QFETCH(const QMediaFormat::FileFormat, fileFormat);
        QFETCH(const QString, inputFileName);
        QFETCH(const QString, expectedFileName);

        QTemporaryDir tempDir;

        const QUrl url = QUrl::fromLocalFile(tempDir.filePath(inputFileName));

        QMediaCaptureSession session;

        QMediaRecorder recorder;
        recorder.setOutputLocation(url);
        recorder.setMediaFormat({ fileFormat });

        QAudioFormat format;
        format.setChannelConfig(QAudioFormat::ChannelConfigMono);
        format.setChannelCount(1);
        format.setSampleFormat(QAudioFormat::Float);
        format.setSampleRate(44000);

        QAudioBufferInput input{ format };
        session.setAudioBufferInput(&input);
        session.setRecorder(&recorder);

        AudioGenerator generator;
        generator.setFormat(format);
        generator.setDuration(1s);
        generator.emitEmptyBufferOnStop();

        bool done = false;
        connect(&recorder, &QMediaRecorder::recorderStateChanged, &recorder,
                [&](QMediaRecorder::RecorderState state) {
                    if (state == QMediaRecorder::StoppedState)
                        done = true;
                });

        connect(&input, &QAudioBufferInput::readyToSendAudioBuffer, //
                &generator, &AudioGenerator::nextBuffer);

        connect(&generator, &AudioGenerator::audioBufferCreated, //
                &input, &QAudioBufferInput::sendAudioBuffer);

        recorder.setAutoStop(true);

        recorder.record();

        QTRY_VERIFY_WITH_TIMEOUT(done, 60s); // Timeout can be as large as possible

        const QUrl loc = recorder.actualLocation();

        const bool pass = loc.toString().endsWith(expectedFileName);
        if (!pass)
            qWarning() << loc << "does not match expected " << expectedFileName;

        QVERIFY(pass);
    }

    void record_writesVideo_whenInputFrameShrinksOverTime()
    {
        CaptureSessionFixture f{ StreamType::Video };
        f.start(RunMode::Push, AutoStop::EmitEmpty);
        f.readyToSendVideoFrame.wait();

        constexpr int startSize = 38;
        int frameCount = 0;
        for (int i = 0; i < startSize;
             i += 2) { // TODO crash in sws_scale if subsequent frames are odd-sized QTBUG-126259
            ++frameCount;
            const QSize size{ startSize - i, startSize - i };
            f.m_videoGenerator.setSize(size);
            f.m_videoInput.sendVideoFrame(f.m_videoGenerator.createFrame());
            f.readyToSendVideoFrame.wait();
        }

        f.m_videoInput.sendVideoFrame({});

        QVERIFY(f.waitForRecorderStopped(60s));
        QVERIFY2(f.m_recorder.error() == QMediaRecorder::NoError,
                 f.m_recorder.errorString().toLatin1());

        auto info = MediaInfo::create(f.m_recorder.actualLocation());

        QCOMPARE_EQ(info->m_frameCount, frameCount);

        // All frames should be resized to the size of the first frame
        QCOMPARE_EQ(info->m_size, QSize(startSize, startSize));
    }

    void record_writesVideo_whenInputFrameGrowsOverTime()
    {
        CaptureSessionFixture f{ StreamType::Video };
        f.start(RunMode::Push, AutoStop::EmitEmpty);
        f.readyToSendVideoFrame.wait();

        constexpr int startSize = 38;
        constexpr int maxSize = 256;
        int frameCount = 0;

        for (int i = 0; i < maxSize - startSize;
             i += 2) { // TODO crash in sws_scale if subsequent frames are odd-sized QTBUG-126259
            ++frameCount;
            const QSize size{ startSize + i, startSize + i };
            f.m_videoGenerator.setPattern(ImagePattern::ColoredSquares);
            f.m_videoGenerator.setSize(size);
            f.m_videoInput.sendVideoFrame(f.m_videoGenerator.createFrame());
            f.readyToSendVideoFrame.wait();
        }

        f.m_videoInput.sendVideoFrame({});

        QVERIFY(f.waitForRecorderStopped(60s));
        QVERIFY2(f.m_recorder.error() == QMediaRecorder::NoError,
                 f.m_recorder.errorString().toLatin1());

        auto info = MediaInfo::create(f.m_recorder.actualLocation());

        QCOMPARE_EQ(info->m_frameCount, frameCount);

        // All frames should be resized to the size of the first frame
        QCOMPARE_EQ(info->m_size, QSize(startSize, startSize));
    }

    void record_stopsRecording_whenInputsReportedEndOfStream_data()
    {
        QTest::addColumn<bool>("audioStopsFirst");

        QTest::addRow("audio stops first") << true;
        QTest::addRow("video stops first") << true;
    }

    void record_stopsRecording_whenInputsReportedEndOfStream()
    {
        QFETCH(const bool, audioStopsFirst);

        CaptureSessionFixture f{ StreamType::AudioAndVideo };
        f.m_recorder.setAutoStop(true);

        f.m_audioGenerator.setBufferCount(30);
        f.m_videoGenerator.setFrameCount(30);

        QSignalSpy audioDone{ &f.m_audioGenerator, &AudioGenerator::done };
        QSignalSpy videoDone{ &f.m_videoGenerator, &VideoGenerator::done };

        f.start(RunMode::Pull, AutoStop::No);

        audioDone.wait();
        videoDone.wait();

        if (audioStopsFirst) {
            f.m_audioInput.sendAudioBuffer({});
            QVERIFY(!f.waitForRecorderStopped(300ms)); // Should not stop until both streams stopped
            QVERIFY2(f.m_recorder.error() == QMediaRecorder::NoError,
                     f.m_recorder.errorString().toLatin1());
            f.m_videoInput.sendVideoFrame({});
        } else {
            f.m_videoInput.sendVideoFrame({});
            QVERIFY(!f.waitForRecorderStopped(300ms)); // Should not stop until both streams stopped
            QVERIFY2(f.m_recorder.error() == QMediaRecorder::NoError,
                     f.m_recorder.errorString().toLatin1());
            f.m_audioInput.sendAudioBuffer({});
        }

        QVERIFY(f.waitForRecorderStopped(60s));
        QVERIFY2(f.m_recorder.error() == QMediaRecorder::NoError,
                 f.m_recorder.errorString().toLatin1());

        // check if the file has been written

        const std::optional<MediaInfo> mediaInfo = MediaInfo::create(f.m_recorder.actualLocation());

        QVERIFY(mediaInfo);
        QVERIFY(mediaInfo->m_hasVideo);
        QVERIFY(mediaInfo->m_hasAudio);
    }

    void record_writesVideo_withoutTransforms_whenPresentationTransformsPresent_data()
    {
        QTest::addColumn<QtVideo::Rotation>("presentationRotation");
        QTest::addColumn<bool>("presentationMirrored");

        QTest::addRow("No rotation, not mirrored") << QtVideo::Rotation::None << false;
        QTest::addRow("90 degrees, not mirrored") << QtVideo::Rotation::Clockwise90 << false;
        QTest::addRow("180 degrees, not mirrored") << QtVideo::Rotation::Clockwise180 << false;
        QTest::addRow("270 degrees, not mirrored") << QtVideo::Rotation::Clockwise270 << false;
        QTest::addRow("No rotation, mirrored") << QtVideo::Rotation::None << true;
        QTest::addRow("90 degrees, mirrored") << QtVideo::Rotation::Clockwise90 << true;
        QTest::addRow("180 degrees, mirrored") << QtVideo::Rotation::Clockwise180 << true;
        QTest::addRow("270 degrees, mirrored") << QtVideo::Rotation::Clockwise270 << true;
    }

    void record_writesVideo_withoutTransforms_whenPresentationTransformsPresent()
    {
        QFETCH(const QtVideo::Rotation, presentationRotation);
        QFETCH(const bool, presentationMirrored);

        CaptureSessionFixture f{ StreamType::Video };
        f.m_videoGenerator.setPattern(ImagePattern::ColoredSquares);
        f.m_videoGenerator.setFrameCount(3);

        f.m_videoGenerator.setPresentationRotation(presentationRotation);
        f.m_videoGenerator.setPresentationMirrored(presentationMirrored);

        f.start(RunMode::Pull, AutoStop::EmitEmpty);
        QVERIFY(f.waitForRecorderStopped(60s));
        QVERIFY2(f.m_recorder.error() == QMediaRecorder::NoError,
                 f.m_recorder.errorString().toLatin1());

        const auto info = MediaInfo::create(f.m_recorder.actualLocation());
        QCOMPARE_EQ(info->m_colors.size(), 3);

        std::array<QColor, 4> colors = info->m_colors.front();
        QVERIFY(fuzzyCompare(colors[0], Qt::red));
        QVERIFY(fuzzyCompare(colors[1], Qt::green));
        QVERIFY(fuzzyCompare(colors[2], Qt::blue));
        QVERIFY(fuzzyCompare(colors[3], Qt::yellow));
    }

    void record_writesVideo_withCorrectColors_data()
    {
        QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

        for (int i = QVideoFrameFormat::Format_ARGB8888; i < QVideoFrameFormat::NPixelFormats;
             ++i) {
            const auto format = static_cast<QVideoFrameFormat::PixelFormat>(i);
            if (!isSupportedPixelFormat(format))
                continue;
            const QByteArray formatName = QVideoFrameFormat::pixelFormatToString(format).toLatin1();
            QTest::addRow("%s", formatName.data()) << format;
        }
    }

    void record_writesVideo_withCorrectColors()
    {
        QFETCH(const QVideoFrameFormat::PixelFormat, pixelFormat);

        // Arrange
        CaptureSessionFixture f{ StreamType::Video };
        f.m_videoGenerator.setPixelFormat(pixelFormat);
        f.m_videoGenerator.setPattern(ImagePattern::ColoredSquares);
        f.m_videoGenerator.setFrameCount(1);
        f.m_videoGenerator.setSize({ 128, 64 }); // Small frames to speed up test

        f.start(RunMode::Push, AutoStop::EmitEmpty);

        // Act: Push one frame through and send sentinel stop frame
        f.readyToSendVideoFrame.wait();
        f.m_videoGenerator.nextFrame();
        f.readyToSendVideoFrame.wait();
        f.m_videoGenerator.nextFrame();

        QVERIFY(f.waitForRecorderStopped(60s));
        QVERIFY2(f.m_recorder.error() == QMediaRecorder::NoError,
                 f.m_recorder.errorString().toLatin1());

        const auto info = MediaInfo::create(f.m_recorder.actualLocation(), /*keep frames */ true);

        const QImage expectedImage = f.m_videoGenerator.createFrame().toImage();

        QCOMPARE_EQ(info->m_frames.size(), 2u); // Front has content, back is empty
        const QImage actualImage = info->m_frames.front().toImage();

        // Store images to simplify debugging/verifying output
        const QString path =
                m_tempDir.filePath(QVideoFrameFormat::pixelFormatToString(pixelFormat));
        QVERIFY(expectedImage.save(path + "_expected.png"));
        QVERIFY(actualImage.save(path + "_actual.png"));

        // Extract center of each quadrant, because recorder compression introduces artifacts
        // in color boundaries.
        const std::array<QColor, 4> expectedColors = MediaInfo::sampleQuadrants(expectedImage);
        const std::array<QColor, 4> actualColors = MediaInfo::sampleQuadrants(actualImage);

        // Assert that colors are similar (not exactly the same because compression introduces minor
        // differences)
        QVERIFY(fuzzyCompare(expectedColors[0], actualColors[0]));
        QVERIFY(fuzzyCompare(expectedColors[1], actualColors[1]));
        QVERIFY(fuzzyCompare(expectedColors[2], actualColors[2]));
        QVERIFY(fuzzyCompare(expectedColors[3], actualColors[3]));
    }

    void actualLocation_returnsNonEmptyLocation_whenRecorderEntersRecordingState()
    {
        const QUrl url = QUrl::fromLocalFile(m_tempDir.filePath("any_file_name"));
        CaptureSessionFixture f{ StreamType::AudioAndVideo };
        f.m_recorder.setOutputLocation(url);

        auto onStateChanged = [&f](QMediaRecorder::RecorderState state) {
            QCOMPARE(state, QMediaRecorder::RecordingState);
            QCOMPARE_NE(f.m_recorder.actualLocation(), QUrl());
        };

        connect(&f.m_recorder, &QMediaRecorder::recorderStateChanged, this, onStateChanged,
                Qt::SingleShotConnection);

        QCOMPARE(f.m_recorder.actualLocation(), QUrl());
        f.start(RunMode::Pull, AutoStop::EmitEmpty);

        QTRY_COMPARE(f.m_recorder.recorderState(), QMediaRecorder::RecordingState);
        f.m_recorder.stop();
    }

private:
    QTemporaryDir m_tempDir;
};

QTEST_MAIN(tst_qmediarecorderbackend)

#include "tst_qmediarecorderbackend.moc"
