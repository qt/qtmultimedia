// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtMultimedia/qmediarecorder.h>
#include <QtMultimedia/qmediacapturesession.h>
#include <QtMultimedia/qaudiobufferinput.h>
#include <QtMultimedia/qmediaformat.h>
#include <private/audiogenerationutils_p.h>
#include <private/mediabackendutils_p.h>

#include <QtCore/qtemporarydir.h>
#include <chrono>

using namespace std::chrono_literals;

QT_USE_NAMESPACE

using namespace Qt::StringLiterals;

class tst_qmediarecorderbackend : public QObject
{
    Q_OBJECT

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
};

QTEST_GUILESS_MAIN(tst_qmediarecorderbackend)
#include "tst_qmediarecorderbackend.moc"
