// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformaudiodecoder_p.h"
#include "qthread.h"

QT_BEGIN_NAMESPACE

QPlatformAudioDecoder::QPlatformAudioDecoder(QAudioDecoder *parent) : q(parent) { }

QPlatformAudioDecoder::~QPlatformAudioDecoder() = default;

void QPlatformAudioDecoder::error(int error, const QString &errorString)
{
    if (error == m_error && errorString == m_errorString)
        return;
    m_error = QAudioDecoder::Error(error);
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
        QMetaObject::invokeMethod(q, "bufferAvailableChanged", Qt::QueuedConnection, Q_ARG(bool, available));
    else
        emit q->bufferAvailableChanged(available);
}

void QPlatformAudioDecoder::bufferReady()
{
    if (!q->thread()->isCurrentThread())
        QMetaObject::invokeMethod(q, "bufferReady", Qt::QueuedConnection);
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
    durationChanged(-1);
    setIsDecoding(false);
    emit q->finished();
}

void QPlatformAudioDecoder::positionChanged(qint64 position)
{
    if (m_position == position)
        return;
    m_position = position;
    emit q->positionChanged(position);
}

void QPlatformAudioDecoder::durationChanged(qint64 duration)
{
    if (m_duration == duration)
        return;
    m_duration = duration;
    emit q->durationChanged(duration);
}

QT_END_NAMESPACE

#include "moc_qplatformaudiodecoder_p.cpp"
