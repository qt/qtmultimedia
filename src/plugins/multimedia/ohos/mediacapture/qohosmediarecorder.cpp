// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosmediarecorder_p.h"

#include "qohoscamerasession_p.h"
#include "qohosmediacapturesession_p.h"

#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qloggingcategory.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qmediadevices.h>

#include <private/qplatformaudioinput_p.h>

#include <fcntl.h>
#include <unistd.h>

#include <cstring>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcOhosMediaRecorder, "qt.multimedia.ohos.mediarecorder")

namespace {

OH_AVRecorder_CodecMimeType audioCodecToOhos(QMediaFormat::AudioCodec codec)
{
    switch (codec) {
    case QMediaFormat::AudioCodec::MP3:
        return AVRECORDER_AUDIO_MP3;
    case QMediaFormat::AudioCodec::AAC:
    case QMediaFormat::AudioCodec::Unspecified:
    default:
        return AVRECORDER_AUDIO_AAC;
    }
}

OH_AVRecorder_ContainerFormatType containerToOhos(QMediaFormat::FileFormat fmt)
{
    switch (fmt) {
    case QMediaFormat::AAC:
        return AVRECORDER_CFT_AAC;
    case QMediaFormat::MP3:
        return AVRECORDER_CFT_MP3;
    case QMediaFormat::Wave:
        return AVRECORDER_CFT_WAV;
    case QMediaFormat::Mpeg4Audio:
        return AVRECORDER_CFT_MPEG_4A;
    case QMediaFormat::MPEG4:
    case QMediaFormat::UnspecifiedFormat:
    default:
        return AVRECORDER_CFT_MPEG_4;
    }
}

int qualityToAudioBitrate(QMediaRecorder::Quality q)
{
    switch (q) {
    case QMediaRecorder::VeryLowQuality: return 32000;
    case QMediaRecorder::LowQuality:     return 64000;
    case QMediaRecorder::HighQuality:    return 192000;
    case QMediaRecorder::VeryHighQuality:return 256000;
    case QMediaRecorder::NormalQuality:
    default:                             return 128000;
    }
}

} // namespace

QOhosMediaRecorder::QOhosMediaRecorder(QMediaRecorder *parent)
    : QObject(parent), QPlatformMediaRecorder(parent)
{
    m_audioOnlyDurationTimer.setInterval(100);
    connect(&m_audioOnlyDurationTimer, &QTimer::timeout, this, [this] {
        if (state() == QMediaRecorder::RecordingState)
            durationChanged(duration());
    });
}

QOhosMediaRecorder::~QOhosMediaRecorder()
{
    destroyWaveRecorder();
    destroyAudioOnlyRecorder();
}

bool QOhosMediaRecorder::isLocationWritable(const QUrl &location) const
{
    return location.isValid() && (location.isLocalFile() || location.isRelative());
}

QMediaRecorder::RecorderState QOhosMediaRecorder::state() const
{
    if (m_waveSource)
        return m_waveState;
    if (m_audioOnlyRecorder)
        return m_audioOnlyState;
    return m_session ? m_session->recorderState() : QMediaRecorder::StoppedState;
}

qint64 QOhosMediaRecorder::duration() const
{
    if (m_waveSource) {
        if (m_waveState == QMediaRecorder::StoppedState)
            return 0;
        if (m_waveState == QMediaRecorder::PausedState)
            return m_wavePausedMs;
        if (!m_waveTimer.isValid())
            return m_wavePausedMs;
        return m_wavePausedMs + (m_waveTimer.elapsed() - m_waveResumeStartMs);
    }
    if (m_audioOnlyRecorder) {
        if (m_audioOnlyState == QMediaRecorder::StoppedState)
            return 0;
        if (m_audioOnlyState == QMediaRecorder::PausedState)
            return m_audioOnlyPausedMs;
        if (!m_audioOnlyTimer.isValid())
            return m_audioOnlyPausedMs;
        return m_audioOnlyPausedMs
                + (m_audioOnlyTimer.elapsed() - m_audioOnlyResumeStartMs);
    }
    return m_session ? m_session->recorderDuration() : 0;
}

void QOhosMediaRecorder::record(QMediaEncoderSettings &settings)
{
    if (!m_service) {
        updateError(QMediaRecorder::ResourceError,
                    tr("Recorder has no capture session attached"));
        return;
    }
    settings.resolveFormat();
    const QString location = findActualLocation(settings);
    if (location.isEmpty()) {
        updateError(QMediaRecorder::ResourceError, tr("No writable output location"));
        return;
    }

    if (m_session && m_session->isActive()) {
        m_session->startRecording(settings, location);
        m_audioOnlyDurationTimer.start();
        return;
    }

    // No camera available — audio-only requires an audio input on the session.
    if (!m_service->audioInput()) {
        updateError(QMediaRecorder::ResourceError,
                    tr("No audio or video input is attached to the capture session"));
        return;
    }

    // OH_AVRecorder has no PCM codec, so Wave output is implemented by piping
    // OH_AudioCapturer frames into a hand-written RIFF/WAVE file.
    if (settings.audioCodec() == QMediaFormat::AudioCodec::Wave
        || settings.fileFormat() == QMediaFormat::Wave) {
        if (recordWave(settings, location))
            m_audioOnlyDurationTimer.start();
        else
            destroyWaveRecorder();
        return;
    }

    if (recordAudioOnly(settings, location))
        m_audioOnlyDurationTimer.start();
    else
        destroyAudioOnlyRecorder();
}

void QOhosMediaRecorder::pause()
{
    if (m_waveSource) {
        pauseWave();
        return;
    }
    if (m_audioOnlyRecorder) {
        pauseAudioOnly();
        return;
    }
    if (m_session)
        m_session->pauseRecording();
}

void QOhosMediaRecorder::resume()
{
    if (m_waveSource) {
        resumeWave();
        return;
    }
    if (m_audioOnlyRecorder) {
        resumeAudioOnly();
        return;
    }
    if (m_session)
        m_session->resumeRecording();
}

void QOhosMediaRecorder::stop()
{
    m_audioOnlyDurationTimer.stop();
    if (m_waveSource) {
        stopWave();
        return;
    }
    if (m_audioOnlyRecorder) {
        stopAudioOnly();
        return;
    }
    if (m_session)
        m_session->stopRecording();
}

void QOhosMediaRecorder::setMetaData(const QMediaMetaData &metaData)
{
    if (m_metaData == metaData)
        return;
    m_metaData = metaData;
    metaDataChanged();
}

void QOhosMediaRecorder::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    auto *ohosSession = static_cast<QOhosMediaCaptureSession *>(session);
    QOhosCameraSession *newCameraSession = ohosSession ? ohosSession->cameraSession() : nullptr;
    if (m_service == ohosSession && m_session == newCameraSession)
        return;

    // Detach from any running recording — losing the session means we have to stop.
    if (m_waveSource)
        stopWave();
    if (m_audioOnlyRecorder)
        stopAudioOnly();
    else if (m_session
             && m_session->recorderState() != QMediaRecorder::StoppedState
             && m_session != newCameraSession)
        m_session->stopRecording();

    disconnectFromSession();
    m_service = ohosSession;
    m_session = newCameraSession;
    if (m_session)
        connectToSession();
}

void QOhosMediaRecorder::onRecorderStateChanged(int state)
{
    stateChanged(static_cast<QMediaRecorder::RecorderState>(state));
}

void QOhosMediaRecorder::onRecorderError(int code, const QString &message)
{
    updateError(static_cast<QMediaRecorder::Error>(code), message);
}

void QOhosMediaRecorder::onRecorderDuration(qint64 ms)
{
    durationChanged(ms);
}

void QOhosMediaRecorder::onRecorderActualLocation(const QUrl &url)
{
    actualLocationChanged(url);
}

void QOhosMediaRecorder::connectToSession()
{
    if (!m_session)
        return;
    connect(m_session, &QOhosCameraSession::recorderStateChanged, this,
            &QOhosMediaRecorder::onRecorderStateChanged);
    connect(m_session, &QOhosCameraSession::recorderErrorOccurred, this,
            &QOhosMediaRecorder::onRecorderError);
    connect(m_session, &QOhosCameraSession::recorderDurationChanged, this,
            &QOhosMediaRecorder::onRecorderDuration);
    connect(m_session, &QOhosCameraSession::recorderActualLocationChanged, this,
            &QOhosMediaRecorder::onRecorderActualLocation);
}

void QOhosMediaRecorder::disconnectFromSession()
{
    if (m_session)
        disconnect(m_session, nullptr, this, nullptr);
}

bool QOhosMediaRecorder::recordAudioOnly(const QMediaEncoderSettings &settings,
                                          const QString &location)
{
    m_audioOnlyRecorder = OH_AVRecorder_Create();
    if (!m_audioOnlyRecorder) {
        updateError(QMediaRecorder::ResourceError, tr("OH_AVRecorder_Create failed"));
        return false;
    }

    OH_AVRecorder_SetStateCallback(m_audioOnlyRecorder, audioOnlyStateCallback, this);
    OH_AVRecorder_SetErrorCallback(m_audioOnlyRecorder, audioOnlyErrorCallback, this);

    QString resolved = location;
    if (QFileInfo(resolved).suffix().isEmpty()) {
        const QString suffix = settings.preferredSuffix();
        if (!suffix.isEmpty())
            resolved.append(QLatin1Char('.')).append(suffix);
        else
            resolved.append(QStringLiteral(".m4a"));
    }

    m_audioOnlyFd = ::open(QFile::encodeName(resolved).constData(),
                           O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (m_audioOnlyFd < 0) {
        updateError(QMediaRecorder::ResourceError,
                    tr("Could not open output file: %1").arg(resolved));
        return false;
    }
    QByteArray urlBytes = QStringLiteral("fd://").toUtf8();
    urlBytes.append(QByteArray::number(m_audioOnlyFd));

    OH_AVRecorder_Config config{};
    config.audioSourceType = AVRECORDER_MIC;
    config.profile.audioBitrate = settings.audioBitRate() > 0
            ? settings.audioBitRate() : qualityToAudioBitrate(settings.quality());
    config.profile.audioChannels = settings.audioChannelCount() > 0
            ? settings.audioChannelCount() : 2;
    config.profile.audioCodec = audioCodecToOhos(settings.audioCodec());
    config.profile.audioSampleRate = settings.audioSampleRate() > 0
            ? settings.audioSampleRate() : 48000;
    config.profile.fileFormat = containerToOhos(settings.fileFormat());
    config.profile.isHdr = false;
    config.profile.enableTemporalScale = false;
    config.url = const_cast<char *>(urlBytes.constData());
    config.fileGenerationMode = AVRECORDER_APP_CREATE;
    config.maxDuration = 0;

    if (auto prepResult = OH_AVRecorder_Prepare(m_audioOnlyRecorder, &config);
        prepResult != AV_ERR_OK) {
        qCWarning(qLcOhosMediaRecorder) << "OH_AVRecorder_Prepare failed code:" << prepResult;
        updateError(QMediaRecorder::FormatError, tr("OH_AVRecorder_Prepare failed"));
        return false;
    }

    if (auto startResult = OH_AVRecorder_Start(m_audioOnlyRecorder); startResult != AV_ERR_OK) {
        qCWarning(qLcOhosMediaRecorder) << "OH_AVRecorder_Start failed code:" << startResult;
        updateError(QMediaRecorder::ResourceError, tr("OH_AVRecorder_Start failed"));
        return false;
    }

    m_audioOnlyActualLocation = QUrl::fromLocalFile(resolved);
    actualLocationChanged(m_audioOnlyActualLocation);
    m_audioOnlyPausedMs = 0;
    m_audioOnlyResumeStartMs = 0;
    m_audioOnlyTimer.restart();
    return true;
}

void QOhosMediaRecorder::stopAudioOnly()
{
    if (!m_audioOnlyRecorder)
        return;
    OH_AVRecorder_Stop(m_audioOnlyRecorder);
    destroyAudioOnlyRecorder();
}

void QOhosMediaRecorder::pauseAudioOnly()
{
    if (!m_audioOnlyRecorder || m_audioOnlyState != QMediaRecorder::RecordingState)
        return;
    if (OH_AVRecorder_Pause(m_audioOnlyRecorder) == AV_ERR_OK)
        m_audioOnlyPausedMs += (m_audioOnlyTimer.elapsed() - m_audioOnlyResumeStartMs);
}

void QOhosMediaRecorder::resumeAudioOnly()
{
    if (!m_audioOnlyRecorder || m_audioOnlyState != QMediaRecorder::PausedState)
        return;
    if (OH_AVRecorder_Resume(m_audioOnlyRecorder) == AV_ERR_OK)
        m_audioOnlyResumeStartMs = m_audioOnlyTimer.elapsed();
}

void QOhosMediaRecorder::destroyAudioOnlyRecorder()
{
    m_audioOnlyDurationTimer.stop();
    if (m_audioOnlyRecorder) {
        OH_AVRecorder_Release(m_audioOnlyRecorder);
        m_audioOnlyRecorder = nullptr;
    }
    if (m_audioOnlyFd >= 0) {
        ::close(m_audioOnlyFd);
        m_audioOnlyFd = -1;
    }
    if (m_audioOnlyState != QMediaRecorder::StoppedState) {
        m_audioOnlyState = QMediaRecorder::StoppedState;
        stateChanged(QMediaRecorder::StoppedState);
    }
    m_audioOnlyTimer.invalidate();
    m_audioOnlyPausedMs = 0;
    m_audioOnlyResumeStartMs = 0;
}

void QOhosMediaRecorder::audioOnlyStateCallback(OH_AVRecorder * /*recorder*/,
                                                 OH_AVRecorder_State state,
                                                 OH_AVRecorder_StateChangeReason /*reason*/,
                                                 void *userData)
{
    auto *self = static_cast<QOhosMediaRecorder *>(userData);
    if (!self)
        return;
    QMetaObject::invokeMethod(
            self, [self, state] { self->onAudioOnlyStateNotification(int(state)); },
            Qt::QueuedConnection);
}

void QOhosMediaRecorder::audioOnlyErrorCallback(OH_AVRecorder * /*recorder*/, int32_t errorCode,
                                                 const char *errorMsg, void *userData)
{
    auto *self = static_cast<QOhosMediaRecorder *>(userData);
    if (!self)
        return;
    const QString message = QString::fromUtf8(errorMsg ? errorMsg : "");
    QMetaObject::invokeMethod(
            self, [self, errorCode, message] {
                self->onAudioOnlyErrorNotification(errorCode, message);
            },
            Qt::QueuedConnection);
}

void QOhosMediaRecorder::onAudioOnlyStateNotification(int state)
{
    QMediaRecorder::RecorderState mapped = m_audioOnlyState;
    switch (state) {
    case AVRECORDER_STARTED:
        mapped = QMediaRecorder::RecordingState;
        break;
    case AVRECORDER_PAUSED:
        mapped = QMediaRecorder::PausedState;
        break;
    case AVRECORDER_STOPPED:
    case AVRECORDER_IDLE:
    case AVRECORDER_RELEASED:
    case AVRECORDER_ERROR:
        mapped = QMediaRecorder::StoppedState;
        break;
    default:
        return;
    }
    if (mapped == m_audioOnlyState)
        return;
    m_audioOnlyState = mapped;
    stateChanged(mapped);
    durationChanged(duration());
}

void QOhosMediaRecorder::onAudioOnlyErrorNotification(int code, const QString &message)
{
    updateError(QMediaRecorder::ResourceError,
                message.isEmpty() ? tr("Recorder error %1").arg(code) : message);
}

namespace {

// 44-byte RIFF/WAVE header for linear PCM. Sizes use placeholders that we
// rewrite once the data chunk is finalised.
void writeWavHeader(QFile &out, int channels, int sampleRate, int bitsPerSample)
{
    const auto write = [&](const char *s) { out.write(s, 4); };
    const auto writeU32 = [&](quint32 v) {
        char b[4]{
            char(v & 0xff), char((v >> 8) & 0xff),
            char((v >> 16) & 0xff), char((v >> 24) & 0xff),
        };
        out.write(b, 4);
    };
    const auto writeU16 = [&](quint16 v) {
        char b[2]{ char(v & 0xff), char((v >> 8) & 0xff) };
        out.write(b, 2);
    };
    write("RIFF");
    writeU32(0); // chunkSize, patched on stop
    write("WAVE");
    write("fmt ");
    writeU32(16); // subChunk1Size
    writeU16(1);  // AudioFormat = PCM
    writeU16(quint16(channels));
    writeU32(quint32(sampleRate));
    writeU32(quint32(sampleRate * channels * (bitsPerSample / 8)));
    writeU16(quint16(channels * (bitsPerSample / 8)));
    writeU16(quint16(bitsPerSample));
    write("data");
    writeU32(0); // subChunk2Size, patched on stop
}

void patchWavSizes(QFile &file, qint64 dataBytes)
{
    file.seek(4);
    quint32 chunkSize = quint32(36 + dataBytes);
    char b[4]{
        char(chunkSize & 0xff), char((chunkSize >> 8) & 0xff),
        char((chunkSize >> 16) & 0xff), char((chunkSize >> 24) & 0xff),
    };
    file.write(b, 4);
    file.seek(40);
    quint32 dataSize = quint32(dataBytes);
    char d[4]{
        char(dataSize & 0xff), char((dataSize >> 8) & 0xff),
        char((dataSize >> 16) & 0xff), char((dataSize >> 24) & 0xff),
    };
    file.write(d, 4);
}

int wavBitsForSampleFormat(QAudioFormat::SampleFormat f)
{
    switch (f) {
    case QAudioFormat::UInt8: return 8;
    case QAudioFormat::Int16: return 16;
    case QAudioFormat::Int32: return 32;
    case QAudioFormat::Float: return 32;
    case QAudioFormat::Unknown:
    case QAudioFormat::NSampleFormats: break;
    }
    return 16;
}

} // namespace

bool QOhosMediaRecorder::recordWave(const QMediaEncoderSettings &settings,
                                     const QString &location)
{
    QString resolved = location;
    if (QFileInfo(resolved).suffix().isEmpty())
        resolved.append(QStringLiteral(".wav"));

    m_waveFormat = QAudioFormat{};
    m_waveFormat.setSampleRate(settings.audioSampleRate() > 0 ? settings.audioSampleRate()
                                                              : 48000);
    m_waveFormat.setChannelCount(settings.audioChannelCount() > 0 ? settings.audioChannelCount()
                                                                  : 2);
    m_waveFormat.setSampleFormat(QAudioFormat::Int16);
    m_waveFormat.setChannelConfig(
            QAudioFormat::defaultChannelConfigForChannelCount(m_waveFormat.channelCount()));

    const QAudioDevice inputDevice = m_service && m_service->audioInput()
            ? m_service->audioInput()->device
            : QMediaDevices::defaultAudioInput();
    if (inputDevice.isNull()) {
        updateError(QMediaRecorder::ResourceError, tr("No audio input device available"));
        return false;
    }

    auto file = std::make_unique<QFile>(resolved);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        updateError(QMediaRecorder::ResourceError,
                    tr("Could not open output file: %1").arg(resolved));
        return false;
    }
    writeWavHeader(*file, m_waveFormat.channelCount(), m_waveFormat.sampleRate(),
                   wavBitsForSampleFormat(m_waveFormat.sampleFormat()));

    auto source = std::make_unique<QAudioSource>(inputDevice, m_waveFormat);
    source->start(file.get());
    if (source->error() != QAudio::NoError) {
        updateError(QMediaRecorder::ResourceError, tr("Audio capture start failed"));
        return false;
    }

    m_waveFile = std::move(file);
    m_waveSource = std::move(source);
    m_wavePausedMs = 0;
    m_waveResumeStartMs = 0;
    m_waveTimer.restart();
    m_waveActualLocation = QUrl::fromLocalFile(resolved);
    actualLocationChanged(m_waveActualLocation);
    m_waveState = QMediaRecorder::RecordingState;
    stateChanged(QMediaRecorder::RecordingState);
    return true;
}

void QOhosMediaRecorder::stopWave()
{
    if (!m_waveSource)
        return;
    destroyWaveRecorder();
}

void QOhosMediaRecorder::pauseWave()
{
    if (!m_waveSource || m_waveState != QMediaRecorder::RecordingState)
        return;
    m_waveSource->suspend();
    m_wavePausedMs += (m_waveTimer.elapsed() - m_waveResumeStartMs);
    m_waveState = QMediaRecorder::PausedState;
    stateChanged(QMediaRecorder::PausedState);
}

void QOhosMediaRecorder::resumeWave()
{
    if (!m_waveSource || m_waveState != QMediaRecorder::PausedState)
        return;
    m_waveSource->resume();
    m_waveResumeStartMs = m_waveTimer.elapsed();
    m_waveState = QMediaRecorder::RecordingState;
    stateChanged(QMediaRecorder::RecordingState);
}

void QOhosMediaRecorder::destroyWaveRecorder()
{
    if (m_waveSource) {
        m_waveSource->stop();
        m_waveSource.reset();
    }
    if (m_waveFile) {
        m_waveFile->flush();
        const qint64 dataBytes = qMax<qint64>(0, m_waveFile->size() - 44);
        patchWavSizes(*m_waveFile, dataBytes);
        m_waveFile->close();
        m_waveFile.reset();
    }
    if (m_waveState != QMediaRecorder::StoppedState) {
        m_waveState = QMediaRecorder::StoppedState;
        stateChanged(QMediaRecorder::StoppedState);
    }
    m_waveTimer.invalidate();
    m_wavePausedMs = 0;
    m_waveResumeStartMs = 0;
}

QT_END_NAMESPACE

#include "moc_qohosmediarecorder_p.cpp"
