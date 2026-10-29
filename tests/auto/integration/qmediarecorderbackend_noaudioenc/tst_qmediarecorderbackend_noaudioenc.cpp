// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimediaTestLib/private/capturesessionfixture_p.h>
#include <QtMultimediaTestLib/private/mediabackendutils_p.h>
#include <QtMultimediaTestLib/private/qintegrationtestbase_p.h>
#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtMultimedia/qaudiobufferinput.h>
#include <QtMultimedia/qmediacapturesession.h>
#include <QtMultimedia/qmediaformat.h>
#include <QtMultimedia/qmediarecorder.h>
#include <QtCore/qtemporarydir.h>

#include <optional>

using namespace std::chrono_literals;

using namespace QtMultimediaTest;

namespace {

std::optional<QMediaFormat::FileFormat> firstEncodableFileFormat()
{
    QMediaFormat format;
    const QList<QMediaFormat::FileFormat> fileFormats =
            format.supportedFileFormats(QMediaFormat::Encode);
    if (fileFormats.isEmpty())
        return std::nullopt;
    return fileFormats.front();
}

QAudioFormat monoFloatAudioFormat()
{
    QAudioFormat audioFormat;
    audioFormat.setChannelConfig(QAudioFormat::ChannelConfigMono);
    audioFormat.setSampleFormat(QAudioFormat::Float);
    audioFormat.setSampleRate(44100);
    return audioFormat;
}

} // namespace

// The FFmpeg encoder list is built once, on first use, so the exclusion has to be in the
// environment before the application starts. See main() below.
class tst_QMediaRecorderBackendNoAudioEnc : public QIntegrationTestBase
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        if (!initIntegrationTestCase())
            return;

        QSKIP_IF_NOT_FFMPEG();

        // Fails if QT_FFMPEG_EXCLUDE_ENCODERS in main() misses an audio encoder of this platform
        QVERIFY2(QMediaFormat{}.supportedAudioCodecs(QMediaFormat::Encode).isEmpty(),
                 "Audio encoders remain available. Extend the exclusion list in main().");

        const std::optional<QMediaFormat::FileFormat> fileFormat = firstEncodableFileFormat();
        if (!fileFormat)
            QSKIP("No file format can be encoded");
        m_fileFormat = *fileFormat;
    }

    void record_emitsFormatError_whenNoAudioEncoderAvailable();
    void record_keepsRecording_whenRecordedAgainAfterEncoderInitFailure();

private:
    QTemporaryDir m_tempDir;
    QMediaFormat::FileFormat m_fileFormat = QMediaFormat::UnspecifiedFormat;
};

void tst_QMediaRecorderBackendNoAudioEnc::
        record_emitsFormatError_whenNoAudioEncoderAvailable()
{
    // Arrange
    QMediaCaptureSession session;
    QMediaRecorder recorder;
    recorder.setMediaFormat({ m_fileFormat });

    QAudioBufferInput input{ monoFloatAudioFormat() };
    session.setAudioBufferInput(&input);
    session.setRecorder(&recorder);

    QSignalSpy errorSpy{ &recorder, &QMediaRecorder::errorOccurred };

    // Act
    recorder.record();

    // Assert
    QTRY_VERIFY(!errorSpy.isEmpty());
    QCOMPARE(errorSpy.front().at(0).value<QMediaRecorder::Error>(), QMediaRecorder::FormatError);
    QCOMPARE(recorder.recorderState(), QMediaRecorder::StoppedState);
}

void tst_QMediaRecorderBackendNoAudioEnc::
        record_keepsRecording_whenRecordedAgainAfterEncoderInitFailure()
{
    // Arrange
    CaptureSessionFixture f{ StreamType::Video };
    f.m_recorder.setOutputLocation(QUrl::fromLocalFile(m_tempDir.filePath("init_failure")));
    f.m_recorder.setMediaFormat({ m_fileFormat });

    QAudioBufferInput audioInput{ monoFloatAudioFormat() };
    f.m_session.setAudioBufferInput(&audioInput);
    f.m_session.setRecorder(&f.m_recorder);

    f.m_recorder.record();

    // Act: record video only
    f.m_session.setAudioBufferInput(nullptr);
    f.m_recorder.setMediaFormat({});
    f.m_recorder.setOutputLocation(QUrl::fromLocalFile(m_tempDir.filePath("video")));
    f.start(RunMode::Pull, AutoStop::No);

    // Assert
    QTRY_COMPARE(f.m_recorder.recorderState(), QMediaRecorder::RecordingState);
    QTest::qWait(500);
    QCOMPARE(f.m_recorder.recorderState(), QMediaRecorder::RecordingState);
    QCOMPARE(f.m_recorder.error(), QMediaRecorder::NoError);

    f.m_recorder.stop();
    QVERIFY(f.waitForRecorderStopped(60s));
}

// QTEST_MAIN's main() is renamed so that the environment is set before the application exists
#define main testlib_main
QTEST_MAIN(tst_QMediaRecorderBackendNoAudioEnc)
#undef main

int main(int argc, char *argv[])
{
    // Names of all FFmpeg encoders that Qt maps to a QMediaFormat::AudioCodec
    qputenv("QT_FFMPEG_EXCLUDE_ENCODERS",
            "aac,aac_at,aac_mf,libfdk_aac,ac3,ac3_fixed,ac3_mf,eac3,libmp3lame,libshine,mp3_mf,"
            "flac,alac,alac_at,truehd,opus,libopus,vorbis,libvorbis,pcm_s16le,wmav1,wmav2");
    return testlib_main(argc, argv);
}

#include "tst_qmediarecorderbackend_noaudioenc.moc"
