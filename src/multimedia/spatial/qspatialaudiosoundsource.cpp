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
#include "qspatialaudiosoundsource_p.h"
#include "qspatialaudiolistener.h"
#include "qspatialaudioengine_p.h"
#include "api/resonance_audio_api.h"
#include <qaudiosink.h>
#include <qurl.h>
#include <qdebug.h>
#include <qaudiodecoder.h>

QT_BEGIN_NAMESPACE

void QSpatialAudioSoundSourcePrivate::load(QSpatialAudioSoundSource *q)
{
    decoder.reset(new QAudioDecoder);
    buffers.clear();
    auto *ep = QSpatialAudioEnginePrivate::get(engine);
    QAudioFormat f = ep->format;
    decoder->setAudioFormat(f);
    qDebug() << "using format" << f << decoder->audioFormat();
    decoder->setSource(url);
    QObject::connect(decoder.get(), &QAudioDecoder::bufferReady, q, &QSpatialAudioSoundSource::bufferReady);
    QObject::connect(decoder.get(), &QAudioDecoder::finished, q, &QSpatialAudioSoundSource::finished);
    decoder->start();
}

void QSpatialAudioSoundSourcePrivate::getBuffer(float *buf, int bufSize)
{
    QMutexLocker l(&mutex);
    if (currentBuffer >= buffers.size()) {
        memset(buf, 0, bufSize*sizeof(float));
    } else {
        int s = bufSize;
        float *ff = buf;
        while (s) {
            const QAudioBuffer &b = buffers.at(currentBuffer);
//            qDebug() << s << b.format().sampleRate() << b.format().channelCount() << b.format().sampleFormat();
            int frames = b.frameCount() - bufPos;
            auto *f = b.constData<QAudioBuffer::F32S>() + bufPos;
            int toCopy = qMin(frames, s);
            for (int i = 0; i < toCopy; ++i) {
                ff[i] = (f[i].channels[0] + f[i].channels[1])/2.;
            }
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

QSpatialAudioSoundSource::QSpatialAudioSoundSource(QSpatialAudioEngine *engine)
    : d(new QSpatialAudioSoundSourcePrivate)
{
    setEngine(engine);
}

QSpatialAudioSoundSource::~QSpatialAudioSoundSource()
{
    setEngine(nullptr);
    delete d;
}

void QSpatialAudioSoundSource::setPosition(QVector3D pos)
{
    d->pos = pos;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSourcePosition(d->sourceId, pos.x(), pos.y(), pos.z());
    emit positionChanged();
}

QVector3D QSpatialAudioSoundSource::position() const
{
    return d->pos;
}

void QSpatialAudioSoundSource::setRotation(const QQuaternion &q)
{
    d->rotation = q;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSourceRotation(d->sourceId, q.x(), q.y(), q.z(), q.scalar());
    emit rotationChanged();
}

QQuaternion QSpatialAudioSoundSource::rotation() const
{
    return d->rotation;
}

void QSpatialAudioSoundSource::setVolume(float volume)
{
    if (d->volume == volume)
        return;
    d->volume = volume;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSourceVolume(d->sourceId, d->volume);
    emit volumeChanged();
}

float QSpatialAudioSoundSource::volume() const
{
    return d->volume;
}

void QSpatialAudioSoundSource::setDistanceModel(DistanceModel model)
{
    if (d->distanceModel == model)
        return;
    d->distanceModel = model;

    d->updateDistanceModel();
    emit distanceModelChanged();
}

void QSpatialAudioSoundSourcePrivate::updateDistanceModel()
{
    auto *ep = QSpatialAudioEnginePrivate::get(engine);
    if (!engine || sourceId < 0)
        return;

    vraudio::DistanceRolloffModel dm = vraudio::kLogarithmic;
    switch (distanceModel) {
    case QSpatialAudioSoundSource::DistanceModel_Linear:
        dm = vraudio::kLinear;
        break;
    case QSpatialAudioSoundSource::DistanceModel_ManualAttenutation:
        dm = vraudio::kNone;
        break;
    default:
        break;
    }

    ep->api->SetSourceDistanceModel(sourceId, dm, minDistance, maxDistance);
}

QSpatialAudioSoundSource::DistanceModel QSpatialAudioSoundSource::distanceModel() const
{
    return d->distanceModel;
}

void QSpatialAudioSoundSource::setMinimumDistance(float min)
{
    if (d->minDistance == min)
        return;
    d->minDistance = min;

    d->updateDistanceModel();
    emit minimumDistanceChanged();
}

float QSpatialAudioSoundSource::minimumDistance() const
{
    return d->minDistance;
}

void QSpatialAudioSoundSource::setMaximumDistance(float max)
{
    if (d->maxDistance == max)
        return;
    d->maxDistance = max;

    d->updateDistanceModel();
    emit maximumDistanceChanged();
}

float QSpatialAudioSoundSource::maximumDistance() const
{
    return d->maxDistance;
}

void QSpatialAudioSoundSource::setManualAttenuation(float attenuation)
{
    if (d->manualAttenuation == attenuation)
        return;
    d->manualAttenuation = attenuation;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSourceDistanceAttenuation(d->sourceId, d->manualAttenuation);
    emit manualAttenuationChanged();
}

float QSpatialAudioSoundSource::manualAttenuation() const
{
    return d->manualAttenuation;
}

void QSpatialAudioSoundSource::setOcclusionIntensity(float occlusion)
{
    if (d->occlusionIntensity == occlusion)
        return;
    d->occlusionIntensity = occlusion;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSoundObjectOcclusionIntensity(d->sourceId, d->occlusionIntensity);
    emit occlusionIntensityChanged();
}

float QSpatialAudioSoundSource::occlusionIntensity() const
{
    return d->occlusionIntensity;
}

void QSpatialAudioSoundSource::setDirectivity(float alpha)
{
    alpha = qBound(0., alpha, 1.);
    if (alpha == d->directivity)
        return;
    d->directivity = alpha;

    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSoundObjectDirectivity(d->sourceId, d->directivity, d->directivityOrder);

    emit directivityChanged();
}

float QSpatialAudioSoundSource::directivity() const
{
    return d->directivity;
}

void QSpatialAudioSoundSource::setDirectivityOrder(float order)
{
    order = qMax(order, 1.);
    if (order == d->directivityOrder)
        return;
    d->directivityOrder = order;

    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSoundObjectDirectivity(d->sourceId, d->directivity, d->directivityOrder);

    emit directivityChanged();
}

float QSpatialAudioSoundSource::directivityOrder() const
{
    return d->directivityOrder;
}

void QSpatialAudioSoundSource::setNearFieldGain(float gain)
{
    gain = qBound(0., gain, 1.);
    if (gain == d->nearFieldGain)
        return;
    d->nearFieldGain = gain;

    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSoundObjectNearFieldEffectGain(d->sourceId, d->nearFieldGain);

    emit nearFieldGainChanged();

}

float QSpatialAudioSoundSource::nearFieldGain() const
{
    return d->nearFieldGain;
}

void QSpatialAudioSoundSource::setSource(const QUrl &url)
{
    if (d->url == url)
        return;
    d->url = url;

    d->load(this);
    emit sourceChanged();
}

QUrl QSpatialAudioSoundSource::source() const
{
    return d->url;
}

void QSpatialAudioSoundSource::setEngine(QSpatialAudioEngine *engine)
{
    if (d->engine == engine)
        return;
    auto *ep = QSpatialAudioEnginePrivate::get(engine);

    if (ep)
        ep->removeSpatialSound(this);
    d->engine = engine;

    ep = QSpatialAudioEnginePrivate::get(engine);
    if (ep) {
        ep->addSpatialSound(this);
        ep->api->SetSourcePosition(d->sourceId, d->pos.x(), d->pos.y(), d->pos.z());
        ep->api->SetSourceRotation(d->sourceId, d->rotation.x(), d->rotation.y(), d->rotation.z(), d->rotation.scalar());
        ep->api->SetSourceVolume(d->sourceId, d->volume);
        ep->api->SetSoundObjectDirectivity(d->sourceId, d->directivity, d->directivityOrder);
        ep->api->SetSoundObjectNearFieldEffectGain(d->sourceId, d->nearFieldGain);
        d->updateDistanceModel();
    }
}

QSpatialAudioEngine *QSpatialAudioSoundSource::engine() const
{
    return d->engine;
}

void QSpatialAudioSoundSource::bufferReady()
{
    QMutexLocker l(&d->mutex);
    auto b = d->decoder->read();
//    qDebug() << "read buffer" << b.format() << b.startTime() << b.duration();
    d->buffers.append(b);
}

void QSpatialAudioSoundSource::finished()
{

}

QT_END_NAMESPACE

#include "moc_qspatialaudiosoundsource.cpp"
