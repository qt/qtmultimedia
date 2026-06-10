// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformaudiodecoder_p.h"

#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

QPlatformAudioDecoder::QPlatformAudioDecoder(QAudioDecoder *parent) : q(parent) { }

QPlatformAudioDecoder::~QPlatformAudioDecoder() = default;

void QPlatformAudioDecoder::error(QAudioDecoder::Error error, const QString &errorString)
{
    if (error == m_error && errorString == m_errorString)
        return;
    m_error = error;
    m_errorString = errorString;

    if (m_error != QAudioDecoder::NoError) {
        setIsDecoding(false);
        emit q->error(m_error);
    }
}

void QPlatformAudioDecoder::bufferAvailableChanged(bool available)
{
    if (m_bufferAvailable == available)
        return;
    m_bufferAvailable = available;

    if (!q->thread()->isCurrentThread())
        QMetaObject::invokeMethod(q, [q = this->q, available] {
            q->bufferAvailableChanged(available);
        }, Qt::QueuedConnection);
    else
        emit q->bufferAvailableChanged(available);
}

void QPlatformAudioDecoder::bufferReady()
{
    if (!q->thread()->isCurrentThread())
        QMetaObject::invokeMethod(q, &QAudioDecoder::bufferReady, Qt::QueuedConnection);
    else
        emit q->bufferReady();
}

void QPlatformAudioDecoder::sourceChanged()
{
    emit q->sourceChanged();
}

void QPlatformAudioDecoder::formatChanged(const QAudioFormat &format)
{
    emit q->formatChanged(format);
}

void QPlatformAudioDecoder::finished()
{
    durationChanged(kInvalidDuration);
    setIsDecoding(false);
    emit q->finished();
}

void QPlatformAudioDecoder::positionChanged(std::chrono::milliseconds position)
{
    if (m_position == position)
        return;
    m_position = position;
    emit q->positionChanged(position.count());
}

void QPlatformAudioDecoder::durationChanged(std::chrono::milliseconds duration)
{
    if (m_duration == duration)
        return;
    m_duration = duration;
    emit q->durationChanged(duration.count());
}

QT_END_NAMESPACE

#include "moc_qplatformaudiodecoder_p.cpp"
