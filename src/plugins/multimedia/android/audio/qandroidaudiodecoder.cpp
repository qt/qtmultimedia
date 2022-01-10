/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qandroidaudiodecoder_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/private/qandroidextras_p.h>
#include <qloggingcategory.h>
#include <QTimer>
#include <QFile>
#include <QDir>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

QT_BEGIN_NAMESPACE

static const char tempFile[] = "encoded.tmp";
static const char tempPath[] = "/storage/emulated/0/data/local/tmp/audiodecoder/";
constexpr int dequeueTimeout = 5000;
Q_LOGGING_CATEGORY(adLogger, "QAndroidAudioDecoder")

Decoder::Decoder()
    : m_format(AMediaFormat_new())
{}

Decoder::~Decoder()
{
    if (m_codec) {
        AMediaCodec_delete(m_codec);
        m_codec = nullptr;
    }

    if (m_extractor) {
        AMediaExtractor_delete(m_extractor);
        m_extractor = nullptr;
    }

    if (m_format) {
        AMediaFormat_delete(m_format);
        m_format = nullptr;
    }
}

void Decoder::stop()
{
    if (!m_codec)
        return;

    const media_status_t err = AMediaCodec_stop(m_codec);
    if (err != AMEDIA_OK)
        qCWarning(adLogger) << "stop() error: " << err;
}

void Decoder::setSource(const QUrl &source)
{
    if (!m_extractor)
        m_extractor = AMediaExtractor_new();

    int fd = -1;
    if (source.path().contains(QLatin1String("content"))) {
        fd = QJniObject::callStaticMethod<jint>("org/qtproject/qt/android/QtNative",
                                "openFdForContentUrl",
                                "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)I",
                                QNativeInterface::QAndroidApplication::context(),
                                QJniObject::fromString(source.path()).object(),
                                QJniObject::fromString(QLatin1String("r")).object());
    } else {
        fd = open(source.path().toStdString().c_str(), O_RDONLY);
    }

    if (fd < 0) {
         emit error(QAudioDecoder::ResourceError, tr("Invalid fileDescriptor for source."));
         return;
     }
    const int size = QFile(source.toString()).size();
    media_status_t status = AMediaExtractor_setDataSourceFd(m_extractor, fd, 0,
                                                            size > 0 ? size : LONG_MAX);
    close(fd);

    if (status != AMEDIA_OK) {
        if (m_extractor) {
            AMediaExtractor_delete(m_extractor);
            m_extractor = nullptr;
        }
        emit error(QAudioDecoder::ResourceError, tr("Setting source for Audio Decoder failed."));
    }
}

void Decoder::createDecoder()
{
    // get encoded format for decoder
    m_format = AMediaExtractor_getTrackFormat(m_extractor, 0);

    const char *mime;
    if (!AMediaFormat_getString(m_format, AMEDIAFORMAT_KEY_MIME, &mime)) {
        if (m_extractor) {
            AMediaExtractor_delete(m_extractor);
            m_extractor = nullptr;
        }
        emit error(QAudioDecoder::FormatError, tr("Format not supported by Audio Decoder."));

        return;
    }

    // get audio duration from source
    int64_t durationUs;
    AMediaFormat_getInt64(m_format, AMEDIAFORMAT_KEY_DURATION, &durationUs);
    emit durationChanged(durationUs / 1000);

    // set default output audio format from input file
    if (!m_outputFormat.isValid()) {
        int32_t sampleRate;
        AMediaFormat_getInt32(m_format, AMEDIAFORMAT_KEY_SAMPLE_RATE, &sampleRate);
        m_outputFormat.setSampleRate(sampleRate);
        int32_t channelCount;
        AMediaFormat_getInt32(m_format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &channelCount);
        m_outputFormat.setChannelCount(channelCount);
        m_outputFormat.setSampleFormat(QAudioFormat::Int16);
    }

    m_codec = AMediaCodec_createDecoderByType(mime);
}

void Decoder::doDecode()
{
    if (!m_extractor) {
        emit error(QAudioDecoder::ResourceError, tr("Cannot decode, source not set."));
        return;
    }

    createDecoder();

    if (!m_codec) {
        emit error(QAudioDecoder::ResourceError, tr("Audio Decoder could not be created."));
        return;
    }

    media_status_t status = AMediaCodec_configure(m_codec, m_format, nullptr /* surface */,
                                        nullptr /* crypto */, 0);

    if (status != AMEDIA_OK) {
        emit error(QAudioDecoder::ResourceError, tr("Audio Decoder failed configuration."));
        return;
    }

    status = AMediaCodec_start(m_codec);
    if (status != AMEDIA_OK) {
        emit error(QAudioDecoder::ResourceError, tr("Audio Decoder failed to start."));
        return;
    }

    AMediaExtractor_selectTrack(m_extractor, 0);

    m_inputEOS = false;
    while (!m_inputEOS) {
        // handle input buffer
        const ssize_t bufferIdx = AMediaCodec_dequeueInputBuffer(m_codec, dequeueTimeout);

        if (bufferIdx >= 0) {
            size_t bufferSize = {};
            uint8_t *buffer = AMediaCodec_getInputBuffer(m_codec, bufferIdx, &bufferSize);
            const int sample = AMediaExtractor_readSampleData(m_extractor, buffer, bufferSize);
            if (sample < 0) {
                m_inputEOS = true;
                break;
            }

            const int64_t presentationTimeUs = AMediaExtractor_getSampleTime(m_extractor);
            AMediaCodec_queueInputBuffer(m_codec, bufferIdx, 0, sample, presentationTimeUs,
                                         m_inputEOS ? AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM : 0);
            AMediaExtractor_advance(m_extractor);

            // handle output buffer
            AMediaCodecBufferInfo info;
            ssize_t idx = AMediaCodec_dequeueOutputBuffer(m_codec, &info, dequeueTimeout);
            if (idx >= 0) {
                if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM)
                    break;

                if (info.size > 0) {
                    size_t bufferSize;
                    const uint8_t *bufferData = AMediaCodec_getOutputBuffer(m_codec, idx,
                                                                            &bufferSize);
                    const QByteArray data((const char*)(bufferData + info.offset), info.size);
                    auto audioBuffer = QAudioBuffer(data, m_outputFormat, presentationTimeUs);
                    if (presentationTimeUs > 0)
                        emit positionChanged(std::move(audioBuffer), presentationTimeUs / 1000);
                    AMediaCodec_releaseOutputBuffer(m_codec, idx, false);
                }
            } else {
                // The outputIndex doubles as a status return if its value is < 0
                switch (idx) {
                case AMEDIACODEC_INFO_TRY_AGAIN_LATER:
                    qCWarning(adLogger) << "dequeueOutputBuffer() status: try again later";
                    break;
                case AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED:
                    qCWarning(adLogger) << "dequeueOutputBuffer() status: output buffers changed";
                    break;
                case AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED:
                    m_format = AMediaCodec_getOutputFormat(m_codec);
                    qCWarning(adLogger) << "dequeueOutputBuffer() status: outputFormat changed";
                    break;
                }
            }
        } else {
            qCWarning(adLogger) <<  "dequeueInputBuffer() status: invalid buffer idx " << bufferIdx;
        }
    }

    emit finished();
}

QAndroidAudioDecoder::QAndroidAudioDecoder(QAudioDecoder *parent)
    : QPlatformAudioDecoder(parent),
      m_decoder(new Decoder())
{
    connect(m_decoder, &Decoder::positionChanged, this, &QAndroidAudioDecoder::positionChanged);
    connect(m_decoder, &Decoder::durationChanged, this, &QAndroidAudioDecoder::durationChanged);
    connect(m_decoder, &Decoder::error, this, &QAndroidAudioDecoder::error);
    connect(m_decoder, &Decoder::finished, this, &QAndroidAudioDecoder::finished);
}

QAndroidAudioDecoder::~QAndroidAudioDecoder()
{
    m_decoder->thread()->exit();
    m_decoder->deleteLater();
}

void QAndroidAudioDecoder::setSource(const QUrl &fileName)
{
    if (!requestPermissions())
        return;

    if (isDecoding())
        return;

    m_device = nullptr;
    error(QAudioDecoder::NoError, QStringLiteral(""));

    if (m_source != fileName) {
        m_source = fileName;
        m_decoder->setSource(m_source);
        sourceChanged();
    }
}

void QAndroidAudioDecoder::setSourceDevice(QIODevice *device)
{
    if (isDecoding())
        return;

    m_source.clear();
    if (m_device != device) {
        m_device = device;

        if (!requestPermissions())
            return;

        sourceChanged();
    }
}

void QAndroidAudioDecoder::start()
{
    if (isDecoding())
        return;

    setIsDecoding(true);
    m_position = -1;

    m_threadDecoder = new QThread(this);
    m_decoder->moveToThread(m_threadDecoder);
    m_threadDecoder->start();
    decode();
}

void QAndroidAudioDecoder::stop()
{
    if (!isDecoding())
        return;

    m_decoder->stop();

    if (m_threadDecoder && m_threadDecoder->isRunning())
        m_threadDecoder->exit();

    QMutexLocker locker(&m_buffersMutex);
    m_position = -1;
    m_audioBuffer.clear();
    locker.unlock();
    setIsDecoding(false);
}

QAudioBuffer QAndroidAudioDecoder::read()
{
    QMutexLocker locker(&m_buffersMutex);
    if (m_buffersAvailable && !m_audioBuffer.isEmpty()) {
        --m_buffersAvailable;
        return m_audioBuffer.takeFirst();
    }

    // no buffers available
    return {};
}

bool QAndroidAudioDecoder::bufferAvailable() const
{
    QMutexLocker locker(&m_buffersMutex);
    return m_buffersAvailable;
}

qint64 QAndroidAudioDecoder::position() const
{
    QMutexLocker locker(&m_buffersMutex);
    return m_position;
}

qint64 QAndroidAudioDecoder::duration() const
{
    QMutexLocker locker(&m_buffersMutex);
    return m_duration;
}

void QAndroidAudioDecoder::positionChanged(QAudioBuffer audioBuffer, qint64 position)
{
    QMutexLocker locker(&m_buffersMutex);
    m_audioBuffer.append(audioBuffer);
    m_position = position;
    m_buffersAvailable++;
    locker.unlock();
    emit bufferReady();
    emit QPlatformAudioDecoder::positionChanged(position);
}

void QAndroidAudioDecoder::durationChanged(qint64 duration)
{
    QMutexLocker locker(&m_buffersMutex);
    m_duration = duration;
    locker.unlock();
    emit QPlatformAudioDecoder::durationChanged(duration);
}

void QAndroidAudioDecoder::error(const QAudioDecoder::Error err, const QString &errorString)
{
    stop();
    emit QPlatformAudioDecoder::error(err, errorString);
}

void QAndroidAudioDecoder::finished()
{
    stop();
    // remove temp file when decoding is finished
    QFile(QString::fromUtf8(tempPath).append(QString::fromUtf8(tempFile))).remove();
    emit QPlatformAudioDecoder::finished();
}

bool QAndroidAudioDecoder::requestPermissions()
{
    const auto writeRes = QtAndroidPrivate::requestPermission(QtAndroidPrivate::Storage);
    if (writeRes.result() == QtAndroidPrivate::Authorized)
        return true;

    return false;
}

void QAndroidAudioDecoder::decode()
{
    if (m_device) {
        connect(m_device, &QIODevice::readyRead, this, &QAndroidAudioDecoder::readDevice);
        if (m_device->bytesAvailable())
            readDevice();
    } else {
        QTimer::singleShot(0, m_decoder, &Decoder::doDecode);
    }
}

bool QAndroidAudioDecoder::createTempFile()
{
    QFile file = QFile(QString::fromUtf8(tempPath).append(QString::fromUtf8(tempFile)));
    if (!QDir().mkpath(QString::fromUtf8(tempPath)) || !file.open(QIODevice::WriteOnly)) {
        emit error(QAudioDecoder::ResourceError,
              QString::fromUtf8("Error while creating or opening tmp file"));
        return false;
    }

    QDataStream out;
    out.setDevice(&file);
    out << m_deviceBuffer;
    file.close();

    m_deviceBuffer.clear();
    m_decoder->setSource(file.fileName());

    return true;
}

void QAndroidAudioDecoder::readDevice() {
    m_deviceBuffer.append(m_device->readAll());
    if (m_device->atEnd()) {
        disconnect(m_device, &QIODevice::readyRead, this, &QAndroidAudioDecoder::readDevice);
        if (!createTempFile()) {
            m_deviceBuffer.clear();
            stop();
            return;
        }
        QTimer::singleShot(0, m_decoder, &Decoder::doDecode);
    }
}

QT_END_NAMESPACE
