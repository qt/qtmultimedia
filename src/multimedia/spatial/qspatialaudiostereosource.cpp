/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Spatial Audio module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-NOGPL2$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qspatialaudiostereosource_p.h"
#include "qspatialaudiolistener.h"
#include "qspatialaudioengine_p.h"
#include "api/resonance_audio_api.h"
#include <qaudiosink.h>
#include <qurl.h>
#include <qdebug.h>
#include <qaudiodecoder.h>

QT_BEGIN_NAMESPACE

void QSpatialAudioStereoSourcePrivate::load(QSpatialAudioStereoSource *q)
{
    decoder.reset(new QAudioDecoder);
    buffers.clear();
    auto *ep = QSpatialAudioEnginePrivate::get(engine);
    QAudioFormat f = ep->format;
    decoder->setAudioFormat(f);
    qDebug() << "using format" << f << decoder->audioFormat();
    decoder->setSource(url);
    QObject::connect(decoder.get(), &QAudioDecoder::bufferReady, q, &QSpatialAudioStereoSource::bufferReady);
    QObject::connect(decoder.get(), &QAudioDecoder::finished, q, &QSpatialAudioStereoSource::finished);
    decoder->start();
}

void QSpatialAudioStereoSourcePrivate::getBuffer(QAudioBuffer::F32S *buf, int bufSize)
{
    QMutexLocker l(&mutex);
    if (currentBuffer >= buffers.size()) {
        memset(buf, 0, bufSize*sizeof(float));
    } else {
        int s = bufSize;
        QAudioBuffer::F32S *ff = buf;
        while (s) {
            const QAudioBuffer &b = buffers.at(currentBuffer);
//            qDebug() << s << b.format().sampleRate() << b.format().channelCount() << b.format().sampleFormat();
            int frames = b.frameCount() - bufPos;
            auto *f = b.constData<QAudioBuffer::F32S>() + bufPos;
            int toCopy = qMin(frames, s);
            memcpy(ff, f, toCopy*sizeof(QAudioBuffer::F32S));
            ff += toCopy;
            s -= toCopy;
            bufPos += toCopy;
            Q_ASSERT(bufPos <= b.frameCount());
            if (bufPos == b.frameCount()) {
                ++currentBuffer;
                bufPos = 0;
            }
            if (currentBuffer == buffers.size()) {
                currentBuffer = 0;
            }
        }
        Q_ASSERT(ff - buf == bufSize);
    }
}

QSpatialAudioStereoSource::QSpatialAudioStereoSource(QSpatialAudioEngine *engine)
    : d(new QSpatialAudioStereoSourcePrivate)
{
    setEngine(engine);
}

QSpatialAudioStereoSource::~QSpatialAudioStereoSource()
{
    setEngine(nullptr);
    delete d;
}

void QSpatialAudioStereoSource::setVolume(float volume)
{
    if (d->volume == volume)
        return;
    d->volume = volume;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSourceVolume(d->sourceId, d->volume);
    emit volumeChanged();
}

float QSpatialAudioStereoSource::volume() const
{
    return d->volume;
}

void QSpatialAudioStereoSource::setSource(const QUrl &url)
{
    if (d->url == url)
        return;
    d->url = url;

    d->load(this);
    emit sourceChanged();
}

QUrl QSpatialAudioStereoSource::source() const
{
    return d->url;
}

void QSpatialAudioStereoSource::setEngine(QSpatialAudioEngine *engine)
{
    if (d->engine == engine)
        return;
    auto *ep = QSpatialAudioEnginePrivate::get(engine);

    if (ep)
        ep->removeStereoSound(this);
    d->engine = engine;

    ep = QSpatialAudioEnginePrivate::get(engine);
    if (ep) {
        ep->addStereoSound(this);
        ep->api->SetSourceVolume(d->sourceId, d->volume);
    }
}

QSpatialAudioEngine *QSpatialAudioStereoSource::engine() const
{
    return d->engine;
}

void QSpatialAudioStereoSource::bufferReady()
{
    QMutexLocker l(&d->mutex);
    auto b = d->decoder->read();
//    qDebug() << "read buffer" << b.format() << b.startTime() << b.duration();
    d->buffers.append(b);
}

void QSpatialAudioStereoSource::finished()
{

}

QT_END_NAMESPACE

#include "moc_qspatialaudiostereosource.cpp"
