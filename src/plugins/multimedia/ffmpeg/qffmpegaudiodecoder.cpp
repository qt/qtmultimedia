/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
//#define DEBUG_DECODER

#include "qffmpegaudiodecoder_p.h"
#include "qffmpegdecoder_p.h"

#define MAX_BUFFERS_IN_QUEUE 4

QT_BEGIN_NAMESPACE


QFFmpegAudioDecoder::QFFmpegAudioDecoder(QAudioDecoder *parent)
    : QPlatformAudioDecoder(parent)
{
}

QFFmpegAudioDecoder::~QFFmpegAudioDecoder()
{
    delete decoder;
}

QUrl QFFmpegAudioDecoder::source() const
{
    return m_url;
}

void QFFmpegAudioDecoder::setSource(const QUrl &fileName)
{
    stop();
    m_sourceDevice = nullptr;

    if (m_url == fileName)
        return;
    m_url = fileName;

    QByteArray url = fileName.toEncoded(QUrl::PreferLocalFile);
    AVFormatContext *context = nullptr;
    int ret = avformat_open_input(&context, url.constData(), nullptr, nullptr);
    if (ret < 0) {
        error(QMediaPlayer::AccessDeniedError, QMediaPlayer::tr("Could not open file"));
        return;
    }
//    decoder = new QFFmpeg::Decoder(this, context);

    emit sourceChanged();
}

QIODevice *QFFmpegAudioDecoder::sourceDevice() const
{
    return m_sourceDevice;
}

void QFFmpegAudioDecoder::setSourceDevice(QIODevice *device)
{
    stop();
    m_url.clear();
    bool isSignalRequired = (m_sourceDevice != device);
    m_sourceDevice = device;
    if (isSignalRequired)
        emit sourceChanged();
}

void QFFmpegAudioDecoder::start()
{
    // ### move to setSource
    if (!m_url.isEmpty()) {
        // ###
    } else if (m_sourceDevice) {
        // ###
    } else {
        return;
    }

    // Set audio format
    // ###

    // start decoding
}

void QFFmpegAudioDecoder::stop()
{
    if (decoder)
        decoder->stop();

    m_position = 0;
    emit positionChanged(0);
    setIsDecoding(false);
}

QAudioFormat QFFmpegAudioDecoder::audioFormat() const
{
    return m_audioFormat;
}

void QFFmpegAudioDecoder::setAudioFormat(const QAudioFormat &format)
{
    if (m_audioFormat == format)
        return;

    m_audioFormat = format;
    emit formatChanged(m_audioFormat);
}

QAudioBuffer QFFmpegAudioDecoder::read()
{
    QMutexLocker locker(&queueMutex);
    if (queue.isEmpty())
        return QAudioBuffer();
    QAudioBuffer b = queue.dequeue();
    m_position = b.startTime() + b.duration();
    return b;
}

bool QFFmpegAudioDecoder::bufferAvailable() const
{
    QMutexLocker locker(&queueMutex);
    return !queue.isEmpty();
}

qint64 QFFmpegAudioDecoder::position() const
{
    return m_position;
}

qint64 QFFmpegAudioDecoder::duration() const
{
     return m_duration;
}

QT_END_NAMESPACE
