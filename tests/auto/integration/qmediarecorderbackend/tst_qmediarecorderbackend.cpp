// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudiobufferinput.h>
#include <QtMultimedia/qmediacapturesession.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/qmediaformat.h>
#include <QtMultimedia/qmediarecorder.h>
#include <QtMultimedia/qwavedecoder.h>
#include <private/audiogenerationutils_p.h>
#include <private/mediabackendutils_p.h>
#include <private/capturesessionfixture_p.h>
#include <private/mediainfo_p.h>
#include <private/qcolorutil_p.h>
#include <private/qfileutil_p.h>
#include <private/mediabackendutils_p.h>
#include <private/formatutils_p.h>
#include <private/osdetection_p.h>

#include <QtCore/qtemporarydir.h>
#include <QtCore/qmimetype.h>
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

std::set<QMediaFormat::VideoCodec> unsupportedVideoCodecs(QMediaFormat::FileFormat fileFormat)
{
    using VideoCodec = QMediaFormat::VideoCodec;

    std::set<QMediaFormat::VideoCodec> unsupportedCodecs;
    if constexpr (isMacOS) {
        if constexpr (isArm) {
            if (fileFormat == QMediaFormat::FileFormat::WMV)
                unsupportedCodecs.insert(VideoCodec::H264);
            else if (fileFormat == QMediaFormat::FileFormat::AVI)
                unsupportedCodecs.insert(VideoCodec::H264);
            else if (fileFormat == QMediaFormat::MPEG4)
                unsupportedCodecs.insert(VideoCodec::H264);
            else if (fileFormat == QMediaFormat::QuickTime)
                unsupportedCodecs.insert(VideoCodec::H264);
        }
    }

    return unsupportedCodecs;
}

} // namespace

using namespace Qt::StringLiterals;

class tst_QMediaRecorderBackend : public QObject
{
    Q_OBJECT

public slots:

    void cleanupTestCase();

private slots:
    void record_createsFileWithExpectedExtension_whenRecordingAudio_data();
    void record_createsFileWithExpectedExtension_whenRecordingAudio();

    void record_writesVideo_whenInputFrameShrinksOverTime();

    void record_writesVideo_whenInputFrameGrowsOverTime();

    void record_stopsRecording_whenInputsReportedEndOfStream_data();
    void record_stopsRecording_whenInputsReportedEndOfStream();

    void record_writesVideo_withoutTransforms_whenPresentationTransformsPresent_data();
    void record_writesVideo_withoutTransforms_whenPresentationTransformsPresent();

    void record_writesVideo_withCorrectColors_data();
    void record_writesVideo_withCorrectColors();

    void actualLocation_returnsNonEmptyLocation_whenRecorderEntersRecordingState();

    void record_writesToOutputDevice_whenWritableOutputDeviceAndLocationAreSet();

    void record_writesToOutputLocation_whenNotWritableOutputDeviceAndLocationAreSet();

    void record_writesVideo_withAllSupportedVideoFormats_data();
    void record_writesVideo_withAllSupportedVideoFormats();

    void record_writesAudio_withAllSupportedAudioFormats_data();
    void record_writesAudio_withAllSupportedAudioFormats();

    void record_emits_mediaformatChanged_whenFormatChanged();

    void stop_stopsRecording_whenInvokedUponRecordingStart();

    void record_reflectsAudioEncoderSetting();

private:
    QTemporaryDir m_tempDir;
};

void tst_QMediaRecorderBackend::cleanupTestCase()
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

void tst_QMediaRecorderBackend::record_createsFileWithExpectedExtension_whenRecordingAudio_data()
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

void tst_QMediaRecorderBackend::record_createsFileWithExpectedExtension_whenRecordingAudio()
{
    QSKIP_IF_NOT_FFMPEG("This test requires APIs that are only implemented with FFmpeg media backend");

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
    format.setSampleRate(44100);

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

void tst_QMediaRecorderBackend::record_writesVideo_whenInputFrameShrinksOverTime()
{
    QSKIP_IF_NOT_FFMPEG();

    CaptureSessionFixture f{ StreamType::Video };
    f.start(RunMode::Push, AutoStop::EmitEmpty);
    f.readyToSendVideoFrame.wait();

    constexpr int startSize = 38;
    int frameCount = 0;
    for (int i = 0; i < startSize; ++i) {
        ++frameCount;
        const QSize size{ startSize - i, startSize - i };
        f.m_videoGenerator.setSize(size);
        QVERIFY(f.m_videoInput.sendVideoFrame(f.m_videoGenerator.createFrame()));
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

void tst_QMediaRecorderBackend::record_writesVideo_whenInputFrameGrowsOverTime()
{
    QSKIP_IF_NOT_FFMPEG();

    CaptureSessionFixture f{ StreamType::Video };
    f.start(RunMode::Push, AutoStop::EmitEmpty);
    f.readyToSendVideoFrame.wait();

    constexpr int startSize = 38;
    constexpr int maxSize = 256;
    int frameCount = 0;

    for (int i = 0; i < maxSize - startSize; ++i) {
        ++frameCount;
        const QSize size{ startSize + i, startSize + i };
        f.m_videoGenerator.setPattern(ImagePattern::ColoredSquares);
        f.m_videoGenerator.setSize(size);
        QVERIFY(f.m_videoInput.sendVideoFrame(f.m_videoGenerator.createFrame()));
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

void tst_QMediaRecorderBackend::record_stopsRecording_whenInputsReportedEndOfStream_data()
{
    QTest::addColumn<bool>("audioStopsFirst");

    QTest::addRow("audio stops first") << true;
    QTest::addRow("video stops first") << true;
}

void tst_QMediaRecorderBackend::record_stopsRecording_whenInputsReportedEndOfStream()
{
    QSKIP_IF_NOT_FFMPEG();

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

void tst_QMediaRecorderBackend::record_writesVideo_withoutTransforms_whenPresentationTransformsPresent_data()
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

void tst_QMediaRecorderBackend::record_writesVideo_withoutTransforms_whenPresentationTransformsPresent()
{
    QSKIP_IF_NOT_FFMPEG();

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
    QCOMPARE_EQ(info->m_colors.size(), 3u);

    std::array<QColor, 4> colors = info->m_colors.front();
    QVERIFY(fuzzyCompare(colors[0], Qt::red));
    QVERIFY(fuzzyCompare(colors[1], Qt::green));
    QVERIFY(fuzzyCompare(colors[2], Qt::blue));
    QVERIFY(fuzzyCompare(colors[3], Qt::yellow));
}

void tst_QMediaRecorderBackend::record_writesVideo_withCorrectColors_data()
{
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

    for (int i = QVideoFrameFormat::Format_ARGB8888; i < QVideoFrameFormat::NPixelFormats; ++i) {
        const auto format = static_cast<QVideoFrameFormat::PixelFormat>(i);
        if (!isSupportedPixelFormat(format))
            continue;
        const QByteArray formatName = QVideoFrameFormat::pixelFormatToString(format).toLatin1();
        QTest::addRow("%s", formatName.data()) << format;
    }
}

void tst_QMediaRecorderBackend::record_writesVideo_withCorrectColors()
{
    QSKIP_IF_NOT_FFMPEG();

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
    const QString path = m_tempDir.filePath(QVideoFrameFormat::pixelFormatToString(pixelFormat));
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

void tst_QMediaRecorderBackend::actualLocation_returnsNonEmptyLocation_whenRecorderEntersRecordingState()
{
    QSKIP_IF_NOT_FFMPEG();

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

void tst_QMediaRecorderBackend::record_writesToOutputDevice_whenWritableOutputDeviceAndLocationAreSet()
{
    QSKIP_IF_NOT_FFMPEG();

    // Arrange
    const QUrl url = QUrl::fromLocalFile(m_tempDir.filePath("file_to_be_not_created.mp4"));
    CaptureSessionFixture f{ StreamType::Audio };
    f.m_recorder.setOutputLocation(url);

    QTemporaryFile tempFile;
    QTEST_ASSERT(tempFile.open());

    f.m_recorder.setOutputDevice(&tempFile);

    // Act
    f.start(RunMode::Pull, AutoStop::EmitEmpty);

    QVERIFY(f.waitForRecorderStopped(60s));
    tempFile.close();

    // Assert
    QVERIFY(!QFileInfo::exists(url.toLocalFile()));
    QCOMPARE(f.m_recorder.actualLocation(), QUrl());
    QCOMPARE_GT(tempFile.size(), 0);
}

void tst_QMediaRecorderBackend::record_writesToOutputLocation_whenNotWritableOutputDeviceAndLocationAreSet()
{
    QSKIP_IF_NOT_FFMPEG();

    // Arrange
    CaptureSessionFixture f{ StreamType::Audio };

    const QUrl url = QUrl::fromLocalFile(m_tempDir.filePath("file_to_be_not_created.mp4"));

    QTemporaryFile tempFile;
    f.m_recorder.setOutputDevice(&tempFile);
    f.m_recorder.setOutputLocation(url);

    // Act
    f.start(RunMode::Pull, AutoStop::EmitEmpty);

    QVERIFY(f.waitForRecorderStopped(60s));
    tempFile.close();

    // Assert
    const QString actualLocation = f.m_recorder.actualLocation().toLocalFile();
    QVERIFY(QFileInfo::exists(actualLocation));
    QCOMPARE_GT(QFileInfo(actualLocation).size(), 0);
    QCOMPARE_NE(f.m_recorder.actualLocation(), QUrl());
    QCOMPARE(tempFile.size(), 0);
}

void tst_QMediaRecorderBackend::record_writesVideo_withAllSupportedVideoFormats_data()
{
    QTest::addColumn<QMediaFormat>("format");

    for (QMediaFormat f : allFileFormats(/*with unspecified*/ true)) {
        for (const QMediaFormat::VideoCodec &c : allVideoCodecs(/*with unspecified*/ true)) {
            f.setVideoCodec(c);

            if (!f.isSupported(QMediaFormat::Encode))
                continue;

            const auto formatName = QMediaFormat::fileFormatName(f.fileFormat()).toLatin1();
            const auto videoCodecName = QMediaFormat::videoCodecName(f.videoCodec()).toLatin1();

            QTest::addRow("%s,%s", formatName.data(), videoCodecName.data()) << f;
        }
    }
}

void tst_QMediaRecorderBackend::record_writesVideo_withAllSupportedVideoFormats()
{
    if (!isFFMPEGPlatform())
        QSKIP("Tested only with FFmpeg backend because other backends don't have the same "
              "format support");

    QFETCH(const QMediaFormat, format);

    CaptureSessionFixture f{ StreamType::Video };

    f.m_recorder.setMediaFormat(format);
    f.m_videoGenerator.setPattern(ImagePattern::ColoredSquares);
    f.m_videoGenerator.setFrameCount(3);
    f.m_videoGenerator.setFrameRate(24);
    f.m_videoGenerator.setSize({ 128, 64 });

    f.start(RunMode::Pull, AutoStop::EmitEmpty);

    const auto actualFormat = f.m_recorder.mediaFormat();

    qDebug() << "Actual format used: " << QMediaFormat::fileFormatName(actualFormat.fileFormat())
             << "/" << QMediaFormat::videoCodecName(actualFormat.videoCodec());

    QVERIFY(f.waitForRecorderStopped(60s));

    if (unsupportedVideoCodecs(actualFormat.fileFormat()).count(actualFormat.videoCodec()))
        QEXPECT_FAIL("", "QTBUG-126276", Abort);

    QVERIFY2(f.m_recorder.error() == QMediaRecorder::NoError,
             f.m_recorder.errorString().toLatin1().data());

    const std::optional<MediaInfo> info = MediaInfo::create(f.m_recorder.actualLocation());
    QVERIFY(info);

    QCOMPARE_GE(info->m_colors.size(),
                2u); // TODO: We loose one frame with some combinations
    QCOMPARE_LE(info->m_colors.size(), 3u);

    std::array<QColor, 4> colors = info->m_colors.front();
    QVERIFY(fuzzyCompare(colors[0], Qt::red));
    QVERIFY(fuzzyCompare(colors[1], Qt::green));
    QVERIFY(fuzzyCompare(colors[2], Qt::blue));
    QVERIFY(fuzzyCompare(colors[3], Qt::yellow));
}

void tst_QMediaRecorderBackend::record_writesAudio_withAllSupportedAudioFormats_data()
{
    QTest::addColumn<QMediaFormat>("format");

    for (QMediaFormat f : allFileFormats(/*with unspecified*/ true)) {
        for (const QMediaFormat::AudioCodec &c : allAudioCodecs(/*with unspecified*/ true)) {
            f.setAudioCodec(c);

            if (!f.isSupported(QMediaFormat::Encode))
                continue;

            const auto formatName = QMediaFormat::fileFormatName(f.fileFormat()).toLatin1();
            const auto audioCodecName = QMediaFormat::audioCodecName(f.audioCodec()).toLatin1();

            QTest::addRow("%s,%s", formatName.data(), audioCodecName.data()) << f;
        }
    }
}

void tst_QMediaRecorderBackend::record_writesAudio_withAllSupportedAudioFormats()
{
    if (!isFFMPEGPlatform())
        QSKIP("Tested only with FFmpeg backend because other backends don't have the same "
              "format support");

    QFETCH(const QMediaFormat, format);

    CaptureSessionFixture f{ StreamType::Audio };
    f.m_recorder.setMediaFormat(format);

    QAudioFormat audioFormat;
    audioFormat.setSampleRate(44100); // TODO: Change to 8000 fails some tests
    audioFormat.setSampleFormat(QAudioFormat::Float);
    audioFormat.setChannelConfig(
            QAudioFormat::ChannelConfigStereo); // TODO: Change to Mono fails some tests
    f.m_audioGenerator.setFormat(audioFormat);

    constexpr seconds expectedDuration = 1s;

    f.m_audioGenerator.setDuration(expectedDuration);
    f.m_audioGenerator.setFrequency(800);

    f.start(RunMode::Pull, AutoStop::EmitEmpty);

    const auto actualFormat = f.m_recorder.mediaFormat();

    qDebug() << "Actual format: " << QMediaFormat::fileFormatName(actualFormat.fileFormat()) << ","
             << QMediaFormat::audioCodecName(actualFormat.audioCodec());

    QVERIFY(f.waitForRecorderStopped(60s));

    QVERIFY2(f.m_recorder.error() == QMediaRecorder::NoError,
             f.m_recorder.errorString().toLatin1().data());

    const std::optional<MediaInfo> info = MediaInfo::create(f.m_recorder.actualLocation());
    QVERIFY(info);

    QCOMPARE_GE(info->m_audioBuffer.duration(),
                std::chrono::microseconds(expectedDuration / 5).count()); // TODO: Fix cut audio

    QCOMPARE_GE(info->m_audioBuffer.byteCount(), 1u); // TODO: Verify with QSineWaveValidator
}

// TODO: Add test that verifies format support with both audio and video in the same recording

void tst_QMediaRecorderBackend::record_emits_mediaformatChanged_whenFormatChanged()
{
    QSKIP_IF_NOT_FFMPEG();

    // Arrange
    CaptureSessionFixture f{ StreamType::Video };
    f.m_videoGenerator.setFrameCount(1);
    f.m_videoGenerator.setSize({ 128, 64 }); // Small frames to speed up test

    QMediaFormat unspecifiedFormat;
    f.m_recorder.setMediaFormat(unspecifiedFormat);

    f.start(RunMode::Pull, AutoStop::EmitEmpty);

    QVERIFY(f.waitForRecorderStopped(60s));
    QVERIFY2(f.m_recorder.error() == QMediaRecorder::NoError,
             f.m_recorder.errorString().toLatin1().data());

    QCOMPARE_EQ(f.mediaFormatChanged.size(), 1);

    const QMediaFormat actualFormat = f.m_recorder.mediaFormat();
    QCOMPARE_NE(actualFormat.fileFormat(), QMediaFormat::UnspecifiedFormat);
    QCOMPARE_NE(actualFormat.videoCodec(), QMediaFormat::VideoCodec::Unspecified);
    QCOMPARE_NE(actualFormat.audioCodec(), QMediaFormat::AudioCodec::Unspecified);
}

void tst_QMediaRecorderBackend::stop_stopsRecording_whenInvokedUponRecordingStart()
{
    QSKIP_IF_NOT_FFMPEG();

    // Arrange
    const QUrl url = QUrl::fromLocalFile(m_tempDir.filePath("any_file_name"));
    CaptureSessionFixture f{ StreamType::AudioAndVideo };
    f.m_recorder.setOutputLocation(url);

    auto onStateChanged = [&f](QMediaRecorder::RecorderState state) {
        if (state == QMediaRecorder::RecordingState)
            f.m_recorder.stop();
    };

    connect(&f.m_recorder, &QMediaRecorder::recorderStateChanged, this, onStateChanged);

    // Act
    f.start(RunMode::Pull, AutoStop::No);

    // Assert
    QTRY_COMPARE(f.m_recorder.recorderState(), QMediaRecorder::StoppedState);
    QList< QList<QVariant> > expectedRecorderStateChangedSignals( { { QMediaRecorder::RecordingState }, { QMediaRecorder::StoppedState } });
    QCOMPARE(f.recorderStateChanged, expectedRecorderStateChangedSignals);
}

void tst_QMediaRecorderBackend::record_reflectsAudioEncoderSetting()
{
    QSKIP_IF_NOT_FFMPEG();

    // Arrange
    CaptureSessionFixture f{ StreamType::Audio };

    QAudioFormat audioFormat;
    audioFormat.setSampleFormat(QAudioFormat::Float);
    audioFormat.setChannelCount(2);
    audioFormat.setSampleRate(44100);
    f.m_audioGenerator.setFormat(audioFormat);

    QMediaFormat fmt{ QMediaFormat::Wave };
    fmt.setAudioCodec(QMediaFormat::AudioCodec::Wave);
    f.m_recorder.setMediaFormat(fmt);
    f.m_recorder.setAudioSampleRate(24000); // nonstandard sampling rate
    f.m_recorder.setAudioChannelCount(1); // mono

    // act
    f.start(RunMode::Pull, AutoStop::EmitEmpty);
    QVERIFY(f.waitForRecorderStopped(60s));

    // Assert
    auto info = MediaInfo::create(f.m_recorder.actualLocation());
    QVERIFY(info);
    QCOMPARE_EQ(info->m_audioBuffer.format().sampleRate(), 24000);
    QCOMPARE_EQ(info->m_audioBuffer.format().channelCount(), 1);
}

QTEST_MAIN(tst_QMediaRecorderBackend)

#include "tst_qmediarecorderbackend.moc"
