/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qplatformaudiodecoder_p.h"
#include "qthread.h"

QT_BEGIN_NAMESPACE

QPlatformAudioDecoder::QPlatformAudioDecoder(QAudioDecoder *parent)
    : QObject(parent),
    q(parent)
{
}

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

    if (QThread::currentThread() != q->thread())
        QMetaObject::invokeMethod(q, "bufferAvailableChanged", Qt::QueuedConnection, Q_ARG(bool, available));
    else
        emit q->bufferAvailableChanged(available);
}

void QPlatformAudioDecoder::bufferReady()
{
    if (QThread::currentThread() != q->thread())
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
