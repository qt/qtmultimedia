// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "capturesessionfixture.h"
#include "tst_qmediaframeinputsbackend.h"

#include "mediainfo.h"
#include <QtTest/QtTest>
#include <qvideoframeinput.h>
#include <qaudiobufferinput.h>
#include <qsignalspy.h>
#include <qmediarecorder.h>
#include <qmediaplayer.h>
#include <private/qplatformmediaintegration_p.h>
#include <private/qplatformaudioresampler_p.h>
#include <../shared/testvideosink.h>
#include "mediabackendutils.h"
#include <../shared/audiogenerationutils.h>

QT_BEGIN_NAMESPACE

namespace {
struct AudioComparisonResult
{
    struct ChannelInfo
    {
        qreal normalizedCrossCorrelation = 0;
        qreal maxDeviation = 0;
        qreal avgDeviation = 0;
    };

    size_t actualSampleCount = 0;
    size_t expectedSampleCount = 0;
    size_t actualSamplesOffset = 0;
    std::vector<ChannelInfo> channelsInfo;

    bool check() const
    {
        return std::all_of(channelsInfo.begin(), channelsInfo.end(), [](const ChannelInfo &info) {
            return info.normalizedCrossCorrelation > 0.96;
        });
    }

    QString toString() const
    {
        QString result;
        QTextStream stream(&result);
        stream << "AudioComparisonResult:";
        stream << "\n\tactualSampleCount: " << actualSampleCount;
        stream << "\n\texpectedSampleCount: " << expectedSampleCount;
        stream << "\n\tactualSamplesOffset: " << actualSamplesOffset;
        int channel = 0;
        for (auto &chInfo : channelsInfo) {
            stream << "\n\tchannel: " << channel++;
            stream << "\n\t\tnormalizedCrossCorrelation: " << chInfo.normalizedCrossCorrelation;
            stream << "\n\t\tmaxDeviation: " << chInfo.maxDeviation;
            stream << "\n\t\tavgDeviation: " << chInfo.avgDeviation;
        }
        stream.flush();
        return result;
    }
};

AudioComparisonResult::ChannelInfo compareChannelAudioData(const float *lhs, const float *rhs,
                                                           quint32 samplesCount, int channel,
                                                           int channelsCount)
{
    AudioComparisonResult::ChannelInfo result;

    qreal crossCorrelation = 0.;
    qreal lhsStandardDeviation = 0.;
    qreal rhsStandardDeviation = 0.;

    qreal deviationsSum = 0.;

    size_t i = channel;
    for (quint32 sample = 0; sample < samplesCount; ++sample, i += channelsCount) {
        crossCorrelation += lhs[i] * rhs[i];
        lhsStandardDeviation += lhs[i] * lhs[i];
        rhsStandardDeviation += rhs[i] * rhs[i];

        const qreal deviation = qAbs(lhs[i] - rhs[i]);
        deviationsSum += deviation;
        result.maxDeviation = qMax(result.maxDeviation, deviation);
    }

    lhsStandardDeviation = sqrt(lhsStandardDeviation);
    rhsStandardDeviation = sqrt(rhsStandardDeviation);

    result.normalizedCrossCorrelation =
            crossCorrelation / (lhsStandardDeviation * rhsStandardDeviation);
    result.avgDeviation = deviationsSum / samplesCount;

    return result;
}

AudioComparisonResult compareAudioData(QSpan<const float> actual, QSpan<const float> expected,
                                       int channelsCount)
{
    AudioComparisonResult result;
    result.actualSampleCount = actual.size() / channelsCount;
    result.expectedSampleCount = expected.size() / channelsCount;

    // can be calculated
    result.actualSamplesOffset = 0;

    const auto samplesCount =
            qMin(result.actualSampleCount - result.actualSamplesOffset, result.expectedSampleCount);

    for (int channel = 0; channel < channelsCount; ++channel)
        result.channelsInfo.push_back(
                compareChannelAudioData(actual.data() + result.actualSamplesOffset * channelsCount,
                                        expected.data(), samplesCount, channel, channelsCount));
    return result;
}

QAudioBuffer convertAudioBuffer(QAudioBuffer buffer, const QAudioFormat &format)
{
    if (format == buffer.format())
        return buffer;

    auto resampler =
            QPlatformMediaIntegration::instance()->createAudioResampler(buffer.format(), format);
    if (!resampler)
        return {};
    return resampler.value()->resample(buffer.constData<char>(), buffer.byteCount());
}

QSpan<const float> toFloatSpan(const QAudioBuffer &buffer)
{
    return QSpan(buffer.constData<const float>(), buffer.byteCount() / sizeof(float));
}

AudioComparisonResult compareAudioData(QAudioBuffer actual, QAudioBuffer expected)
{
    QAudioFormat format = actual.format();
    format.setSampleFormat(QAudioFormat::Float);

    actual = convertAudioBuffer(std::move(actual), format);
    expected = convertAudioBuffer(std::move(expected), format);

    return compareAudioData(toFloatSpan(actual), toFloatSpan(expected), format.channelCount());
}

} // namespace

void tst_QMediaFrameInputsBackend::initTestCase()
{
    QSKIP_GSTREAMER("Not implemented in the gstreamer backend");
}

void tst_QMediaFrameInputsBackend::mediaRecorderWritesAudio_whenAudioFramesInputSends_data()
{
    QTest::addColumn<int>("bufferCount");
    QTest::addColumn<QAudioFormat::SampleFormat>("sampleFormat");
    QTest::addColumn<QAudioFormat::ChannelConfig>("channelConfig");
    QTest::addColumn<int>("sampleRate");
    QTest::addColumn<milliseconds>("duration");

#ifndef Q_OS_WINDOWS // sample rate 8000 is not supported. TODO: investigate.
    QTest::addRow("bufferCount: 20; sampleFormat: Int16; channelConfig: Mono; sampleRate: 8000; "
                  "duration: 1000")
            << 20 << QAudioFormat::Int16 << QAudioFormat::ChannelConfigMono << 8000 << 1000ms;
#endif

    QTest::addRow("bufferCount: 30; sampleFormat: Int32; channelConfig: Stereo; sampleRate: "
                  "12000; duration: 2000")
            << 30 << QAudioFormat::Int32 << QAudioFormat::ChannelConfigStereo << 12000 << 2000ms;

    QTest::addRow("bufferCount: 30; sampleFormat: Int16; channelConfig: Mono; sampleRate: "
                  "40000; duration: 2000")
            << 30 << QAudioFormat::Int16 << QAudioFormat::ChannelConfigMono << 40000 << 2000ms;

    // TODO: investigate fails of channels configuration
    //   QTest::addRow("bufferCount: 10; sampleFormat: UInt8; channelConfig: 2Dot1; sampleRate:
    //   40000; duration: 1500")
    //           << 10 << QAudioFormat::UInt8 << QAudioFormat::ChannelConfig2Dot1 << 40000 << 1500;
    //   QTest::addRow("bufferCount: 10; sampleFormat: Float; channelConfig: 3Dot0; sampleRate:
    //   50000; duration: 2500")
    //           << 40 << QAudioFormat::Float << QAudioFormat::ChannelConfig3Dot0 << 50000 << 2500;
}

void tst_QMediaFrameInputsBackend::mediaRecorderWritesAudio_whenAudioFramesInputSends()
{
    QFETCH(const int, bufferCount);
    QFETCH(const QAudioFormat::SampleFormat, sampleFormat);
    QFETCH(const QAudioFormat::ChannelConfig, channelConfig);
    QFETCH(const int, sampleRate);
    QFETCH(const milliseconds, duration);

    CaptureSessionFixture f{ StreamType::Audio };

    QAudioFormat format;
    format.setSampleFormat(sampleFormat);
    format.setSampleRate(sampleRate);
    format.setChannelConfig(channelConfig);

    f.m_audioGenerator.setFormat(format);
    f.m_audioGenerator.setBufferCount(bufferCount);
    f.m_audioGenerator.setDuration(duration);

    f.start(RunMode::Pull, AutoStop::EmitEmpty);

    QVERIFY(f.waitForRecorderStopped(60s));

    auto info = MediaInfo::create(f.m_recorder.actualLocation());

    QVERIFY(info->m_hasAudio);
    QCOMPARE_GE(info->m_duration, duration - 50ms);
    QCOMPARE_LE(info->m_duration, duration + 50ms);

    QVERIFY(info->m_audioBuffer.isValid());

    microseconds audioDataDuration(
            info->m_audioBuffer.format().durationForBytes(info->m_audioBuffer.byteCount()));

    // TODO: investigate inaccuracies
    QCOMPARE_GT(audioDataDuration, duration - 50ms);
    QCOMPARE_LT(audioDataDuration, duration + 150ms);

    QByteArray sentAudioData = createSineWaveData(format, duration);

    const AudioComparisonResult comparisonResult =
            compareAudioData(info->m_audioBuffer, QAudioBuffer(sentAudioData, format));

    if (format.channelCount() != 1)
        QSKIP("Temporary skip checking audio comparison for channels count > 1");

    QVERIFY2(comparisonResult.check(), comparisonResult.toString().toLatin1().constData());
}

void tst_QMediaFrameInputsBackend::mediaRecorderWritesVideo_whenVideoFramesInputSendsFrames_data()
{
    QTest::addColumn<int>("framesNumber");
    QTest::addColumn<milliseconds>("frameDuration");
    QTest::addColumn<QSize>("resolution");
    QTest::addColumn<bool>("setTimeStamp");

    QTest::addRow("framesNumber: 5; frameRate: 2; resolution: 50x80; with time stamps")
            << 5 << 500ms << QSize(50, 80) << true;
    QTest::addRow("framesNumber: 20; frameRate: 1; resolution: 200x100; with time stamps")
            << 20 << 1000ms << QSize(200, 100) << true;

    QTest::addRow("framesNumber: 20; frameRate: 30; resolution: 200x100; with frame rate")
            << 20 << 250ms << QSize(200, 100) << false;
    QTest::addRow("framesNumber: 60; frameRate: 4; resolution: 200x100; with frame rate")
            << 60 << 24ms << QSize(200, 100) << false;
}

void tst_QMediaFrameInputsBackend::mediaRecorderWritesVideo_whenVideoFramesInputSendsFrames()
{
    QFETCH(const int, framesNumber);
    QFETCH(const milliseconds, frameDuration);
    QFETCH(const QSize, resolution);
    QFETCH(const bool, setTimeStamp);

    CaptureSessionFixture f{ StreamType::Video };
    f.m_videoGenerator.setFrameCount(framesNumber);
    f.m_videoGenerator.setSize(resolution);

    const qreal frameRate = 1e6 / duration_cast<microseconds>(frameDuration).count();
    if (setTimeStamp)
        f.m_videoGenerator.setPeriod(frameDuration);
    else
        f.m_videoGenerator.setFrameRate(frameRate);

    f.start(RunMode::Pull, AutoStop::EmitEmpty);

    QVERIFY(f.waitForRecorderStopped(60s));

    auto info = MediaInfo::create(f.m_recorder.actualLocation());

    QCOMPARE_LT(info->m_frameRate, frameRate * 1.001);
    QCOMPARE_GT(info->m_frameRate, frameRate * 0.999);

    QCOMPARE_LT(info->m_duration, frameDuration * framesNumber * 1.001);
    QCOMPARE_GE(info->m_duration, frameDuration * framesNumber * 0.999);

    QCOMPARE(info->m_size, resolution);
    QCOMPARE_EQ(info->m_frameCount, framesNumber);
}

struct YUV
{
    double Y;
    double U;
    double V;
};

// Poor man's RGB to YUV conversion with BT.709 coefficients
// from https://en.wikipedia.org/wiki/Y%E2%80%B2UV
QVector3D RGBToYUV(const QColor &c)
{
    const float R = c.redF();
    const float G = c.greenF();
    const float B = c.blueF();
    QVector3D yuv;
    yuv[0] = 0.2126f * R + 0.7152f * G + 0.0722f * B;
    yuv[1] = -0.09991f * R - 0.33609f * G + 0.436f * B;
    yuv[2] = 0.615f * R - 0.55861f * G - 0.05639f * B;
    return yuv;
}

// Considers two colors equal if their YUV components are
// pointing in the same direction and have similar luma (Y)
bool fuzzyCompare(const QColor &lhs, const QColor &rhs, float tol = 1e-2)
{
    const QVector3D lhsYuv = RGBToYUV(lhs);
    const QVector3D rhsYuv = RGBToYUV(rhs);
    const float relativeLumaDiff =
            0.5f * std::abs((lhsYuv[0] - rhsYuv[0]) / (lhsYuv[0] + rhsYuv[0]));
    const float colorDiff = QVector3D::crossProduct(lhsYuv, rhsYuv).length();
    return colorDiff < tol && relativeLumaDiff < tol;
}

void tst_QMediaFrameInputsBackend::mediaRecorderWritesVideo_withCorrectColors()
{
    CaptureSessionFixture f{ StreamType::Video };

    f.m_videoGenerator.setPattern(ImagePattern::ColoredSquares);
    f.m_videoGenerator.setFrameCount(3);

    f.start(RunMode::Pull, AutoStop::EmitEmpty);
    QVERIFY(f.waitForRecorderStopped(60s));

    const auto info = MediaInfo::create(f.m_recorder.actualLocation());
    QCOMPARE_EQ(info->m_colors.size(), 3);

    std::array<QColor, 4> colors = info->m_colors.front();
    QVERIFY(fuzzyCompare(colors[0], Qt::red));
    QVERIFY(fuzzyCompare(colors[1], Qt::green));
    QVERIFY(fuzzyCompare(colors[2], Qt::blue));
    QVERIFY(fuzzyCompare(colors[3], Qt::yellow));
}

void tst_QMediaFrameInputsBackend::
        mediaRecorderWritesVideo_withoutTransforms_whenPresentationTransformsPresent_data()
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

void tst_QMediaFrameInputsBackend::
        mediaRecorderWritesVideo_withoutTransforms_whenPresentationTransformsPresent()
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

    const auto info = MediaInfo::create(f.m_recorder.actualLocation());
    QCOMPARE_EQ(info->m_colors.size(), 3);

    std::array<QColor, 4> colors = info->m_colors.front();
    QVERIFY(fuzzyCompare(colors[0], Qt::red));
    QVERIFY(fuzzyCompare(colors[1], Qt::green));
    QVERIFY(fuzzyCompare(colors[2], Qt::blue));
    QVERIFY(fuzzyCompare(colors[3], Qt::yellow));
}

void tst_QMediaFrameInputsBackend::
        sinkReceivesFrameWithTransformParams_whenPresentationTransformPresent_data()
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

void tst_QMediaFrameInputsBackend::
        sinkReceivesFrameWithTransformParams_whenPresentationTransformPresent()
{
    QFETCH(const QtVideo::Rotation, presentationRotation);
    QFETCH(const bool, presentationMirrored);

    // Arrange
    CaptureSessionFixture f{ StreamType::Video };
    f.m_videoGenerator.setPattern(ImagePattern::ColoredSquares);
    f.m_videoGenerator.setFrameCount(2);

    f.m_videoGenerator.setPresentationRotation(presentationRotation);
    f.m_videoGenerator.setPresentationMirrored(presentationMirrored);

    TestVideoSink videoSink{ true /*store frames*/};
    f.setVideoSink(&videoSink);
    f.start(RunMode::Push, AutoStop::No);

    // Act - push two frames
    f.m_videoGenerator.nextFrame();
    f.m_videoGenerator.nextFrame();
    QCOMPARE_EQ(videoSink.m_frameList.size(), 2);

    // Assert
    const QVideoFrame frame = videoSink.m_frameList.back();
    QCOMPARE_EQ(frame.mirrored(), presentationMirrored);
    QCOMPARE_EQ(frame.rotation(), presentationRotation);

    // Note: Frame data is not transformed and QVideoFrame::toImage does not apply
    // transformations. Transformation parameters should be forwarded to rendering
    const std::array<QColor, 4> colors = MediaInfo::sampleQuadrants(frame.toImage());
    QVERIFY(fuzzyCompare(colors[0], Qt::red));
    QVERIFY(fuzzyCompare(colors[1], Qt::green));
    QVERIFY(fuzzyCompare(colors[2], Qt::blue));
    QVERIFY(fuzzyCompare(colors[3], Qt::yellow));
}

void tst_QMediaFrameInputsBackend::mediaRecorderWritesVideo_whenInputFrameShrinksOverTime()
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
    auto info = MediaInfo::create(f.m_recorder.actualLocation());

    QCOMPARE_EQ(info->m_frameCount, frameCount);

    // All frames should be resized to the size of the first frame
    QCOMPARE_EQ(info->m_size, QSize(startSize, startSize));
}

void tst_QMediaFrameInputsBackend::mediaRecorderWritesVideo_whenInputFrameGrowsOverTime()
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
    auto info = MediaInfo::create(f.m_recorder.actualLocation());

    QCOMPARE_EQ(info->m_frameCount, frameCount);

    // All frames should be resized to the size of the first frame
    QCOMPARE_EQ(info->m_size, QSize(startSize, startSize));
}

void tst_QMediaFrameInputsBackend::mediaRecorderWritesVideo_withSingleFrame()
{
    CaptureSessionFixture f{ StreamType::Video };
    f.m_videoGenerator.setFrameCount(1);
    f.m_videoGenerator.setSize({ 640, 480 });
    f.m_videoGenerator.setPeriod(1s);
    f.start(RunMode::Pull, AutoStop::EmitEmpty);

    QVERIFY(f.waitForRecorderStopped(60s));
    auto info = MediaInfo::create(f.m_recorder.actualLocation());

    QCOMPARE_EQ(info->m_frameCount, 1);
    QCOMPARE_EQ(info->m_duration, 1s);
}

void tst_QMediaFrameInputsBackend::mediaRecorderStopsRecording_whenInputsReportedEndOfStream_data()
{
    QTest::addColumn<bool>("audioStopsFirst");

    QTest::addRow("audio stops first") << true;
    QTest::addRow("video stops first") << true;
}

void tst_QMediaFrameInputsBackend::mediaRecorderStopsRecording_whenInputsReportedEndOfStream()
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
        f.m_videoInput.sendVideoFrame({});
    } else {
        f.m_videoInput.sendVideoFrame({});
        QVERIFY(!f.waitForRecorderStopped(300ms)); // Should not stop until both streams stopped
        f.m_audioInput.sendAudioBuffer({});
    }

    QVERIFY(f.waitForRecorderStopped(60s));

    // check if the file has been written

    const std::optional<MediaInfo> mediaInfo = MediaInfo::create(f.m_recorder.actualLocation());

    QVERIFY(mediaInfo);
    QVERIFY(mediaInfo->m_hasVideo);
    QVERIFY(mediaInfo->m_hasAudio);
}

void tst_QMediaFrameInputsBackend::readyToSend_isEmitted_whenRecordingStarts_data()
{
    QTest::addColumn<StreamType>("streamType");
    QTest::addRow("audio") << StreamType::Audio;
    QTest::addRow("video") << StreamType::Video;
    QTest::addRow("audioAndVideo") << StreamType::AudioAndVideo;
}

void tst_QMediaFrameInputsBackend::readyToSend_isEmitted_whenRecordingStarts()
{
    QFETCH(StreamType, streamType);

    CaptureSessionFixture f{ streamType };

    f.start(RunMode::Push, AutoStop::No);

    if (f.hasAudio())
        QTRY_COMPARE_EQ(f.readyToSendAudioBuffer.size(), 1);

    if (f.hasVideo())
        QTRY_COMPARE_EQ(f.readyToSendVideoFrame.size(), 1);
}

void tst_QMediaFrameInputsBackend::readyToSendVideoFrame_isEmitted_whenSendVideoFrameIsCalled()
{
    CaptureSessionFixture f{ StreamType::Video };
    f.start(RunMode::Push, AutoStop::No);

    QVERIFY(f.readyToSendVideoFrame.wait());

    f.m_videoInput.sendVideoFrame(f.m_videoGenerator.createFrame());
    QVERIFY(f.readyToSendVideoFrame.wait());

    f.m_videoInput.sendVideoFrame(f.m_videoGenerator.createFrame());
    QVERIFY(f.readyToSendVideoFrame.wait());
}

void tst_QMediaFrameInputsBackend::readyToSendAudioBuffer_isEmitted_whenSendAudioBufferIsCalled()
{
    CaptureSessionFixture f{ StreamType::Audio };
    f.start(RunMode::Push, AutoStop::No);

    QVERIFY(f.readyToSendAudioBuffer.wait());

    f.m_audioInput.sendAudioBuffer(f.m_audioGenerator.createAudioBuffer());
    QVERIFY(f.readyToSendAudioBuffer.wait());

    f.m_audioInput.sendAudioBuffer(f.m_audioGenerator.createAudioBuffer());
    QVERIFY(f.readyToSendAudioBuffer.wait());
}

void tst_QMediaFrameInputsBackend::readyToSendVideoFrame_isEmittedRepeatedly_whenPullModeIsEnabled()
{
    CaptureSessionFixture f{ StreamType::Video };

    constexpr int expectedSignalCount = 4;
    f.m_videoGenerator.setFrameCount(expectedSignalCount - 1);

    f.start(RunMode::Pull, AutoStop::EmitEmpty);

    f.waitForRecorderStopped(60s);

    QCOMPARE_EQ(f.readyToSendVideoFrame.size(), expectedSignalCount);
}

void tst_QMediaFrameInputsBackend::
        readyToSendAudioBuffer_isEmittedRepeatedly_whenPullModeIsEnabled()
{
    CaptureSessionFixture f{ StreamType::Audio };

    constexpr int expectedSignalCount = 4;
    f.m_audioGenerator.setBufferCount(expectedSignalCount - 1);

    f.start(RunMode::Pull, AutoStop::EmitEmpty);

    f.waitForRecorderStopped(60s);

    QCOMPARE_EQ(f.readyToSendAudioBuffer.size(), expectedSignalCount);
}

void tst_QMediaFrameInputsBackend::
        readyToSendAudioBufferAndVideoFrame_isEmittedRepeatedly_whenPullModeIsEnabled()
{
    CaptureSessionFixture f{ StreamType::AudioAndVideo };

    constexpr int expectedSignalCount = 4;
    f.m_audioGenerator.setBufferCount(expectedSignalCount - 1);
    f.m_videoGenerator.setFrameCount(expectedSignalCount - 1);

    f.start(RunMode::Pull, AutoStop::EmitEmpty);

    f.waitForRecorderStopped(60s);

    QCOMPARE_EQ(f.readyToSendAudioBuffer.size(), expectedSignalCount);
    QCOMPARE_EQ(f.readyToSendVideoFrame.size(), expectedSignalCount);
}

QT_END_NAMESPACE

QT_USE_NAMESPACE

QTEST_MAIN(tst_QMediaFrameInputsBackend)

#include "moc_tst_qmediaframeinputsbackend.cpp"
