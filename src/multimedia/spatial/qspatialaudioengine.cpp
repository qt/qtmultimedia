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
#include <qspatialaudioengine_p.h>
#include <qspatialaudiosoundsource_p.h>
#include <qspatialaudiostereosource_p.h>
#include <api/resonance_audio_api.h>
#include <qmediadevices.h>
#include <qiodevice.h>
#include <qaudiosink.h>
#include <qdebug.h>
#include <qelapsedtimer.h>

QT_BEGIN_NAMESPACE

class QAudioOutputStream : public QIODevice
{
    Q_OBJECT
public:
    explicit QAudioOutputStream(QSpatialAudioEnginePrivate *d)
        : d(d)
    {
        open(QIODevice::ReadOnly);
    }
    ~QAudioOutputStream();

    qint64 readData(char *data, qint64 len) override;

    qint64 writeData(const char *, qint64) override;

    qint64 size() const override { return 0; }
    qint64 bytesAvailable() const override {
        return std::numeric_limits<qint64>::max();
    }
    bool isSequential() const override {
        return true;
    }
    bool atEnd() const override {
        return false;
    }
    qint64 pos() const override {
        return m_pos;
    }

    Q_INVOKABLE void startOutput() {
        QMutexLocker l(&d->mutex);
        Q_ASSERT(!sink);
        QAudioFormat f = d->format;
        f.setSampleFormat(QAudioFormat::Int16);
        sink.reset(new QAudioSink(QMediaDevices::defaultAudioOutput(), f));
        sink->start(this);
    }

    Q_INVOKABLE void stopOutput() {
        sink->stop();
        sink.reset();
        delete this;
    }

private:
    qint64 m_pos = 0;
    QSpatialAudioEnginePrivate *d = nullptr;
    std::unique_ptr<QAudioSink> sink;
};


QAudioOutputStream::~QAudioOutputStream()
{
}

qint64 QAudioOutputStream::writeData(const char *, qint64)
{
    return 0;
}

qint64 QAudioOutputStream::readData(char *data, qint64 len)
{
    if (len < 2*int(sizeof(float))*QSpatialAudioEnginePrivate::bufferSize)
        return 0;

    short *fd = (short *)data;
    int frames = len / 2 / sizeof(short);
    bool ok = true;
    while (frames >= QSpatialAudioEnginePrivate::bufferSize) {
        // Fill input buffers
        for (auto *source : qAsConst(d->sources)) {
            auto *sp = QSpatialAudioSoundSourcePrivate::get(source);
            float buf[QSpatialAudioEnginePrivate::bufferSize];
            sp->getBuffer(buf, QSpatialAudioEnginePrivate::bufferSize);
            d->api->SetInterleavedBuffer(sp->sourceId, buf, 1, QSpatialAudioEnginePrivate::bufferSize);
        }
        for (auto *source : qAsConst(d->stereoSources)) {
            auto *sp = QSpatialAudioStereoSourcePrivate::get(source);
            QAudioBuffer::F32S buf[2*QSpatialAudioEnginePrivate::bufferSize];
            sp->getBuffer(buf, QSpatialAudioEnginePrivate::bufferSize);
            d->api->SetInterleavedBuffer(sp->sourceId, (float *)buf, 2, QSpatialAudioEnginePrivate::bufferSize);
        }

        ok = d->api->FillInterleavedOutputBuffer(2, QSpatialAudioEnginePrivate::bufferSize, fd);
        if (!ok) {
            qWarning() << "    Reading failed!";
            break;
        }
        fd += 2*QSpatialAudioEnginePrivate::bufferSize;
        frames -= QSpatialAudioEnginePrivate::bufferSize;
    }
    const int bytesProcessed = ((char *)fd - data);
    m_pos += bytesProcessed;
    return bytesProcessed;
}


QSpatialAudioEnginePrivate::QSpatialAudioEnginePrivate()
{
}

QSpatialAudioEnginePrivate::~QSpatialAudioEnginePrivate()
{
    delete api;
}

void QSpatialAudioEnginePrivate::addSpatialSound(QSpatialAudioSoundSource *sound)
{
    QSpatialAudioSoundSourcePrivate *sd = QSpatialAudioSoundSourcePrivate::get(sound);

    sd->sourceId = api->CreateSoundObjectSource(vraudio::kBinauralHighQuality);
    sources.append(sound);
}

void QSpatialAudioEnginePrivate::removeSpatialSound(QSpatialAudioSoundSource *sound)
{
    QSpatialAudioSoundSourcePrivate *sd = QSpatialAudioSoundSourcePrivate::get(sound);

    api->DestroySource(sd->sourceId);
    sd->sourceId = vraudio::ResonanceAudioApi::kInvalidSourceId;
    sources.removeOne(sound);
}

void QSpatialAudioEnginePrivate::addStereoSound(QSpatialAudioStereoSource *sound)
{
    QSpatialAudioStereoSourcePrivate *sd = QSpatialAudioStereoSourcePrivate::get(sound);

    sd->sourceId = api->CreateStereoSource(2);
    stereoSources.append(sound);
}

void QSpatialAudioEnginePrivate::removeStereoSound(QSpatialAudioStereoSource *sound)
{
    QSpatialAudioStereoSourcePrivate *sd = QSpatialAudioStereoSourcePrivate::get(sound);

    api->DestroySource(sd->sourceId);
    sd->sourceId = vraudio::ResonanceAudioApi::kInvalidSourceId;
    stereoSources.removeOne(sound);
}

QSpatialAudioEngine::QSpatialAudioEngine(QObject *parent, int sampleRate)
    : QObject(parent)
    , d(new QSpatialAudioEnginePrivate)
{
    d->sampleRate = sampleRate;
    d->api = vraudio::CreateResonanceAudioApi(2, QSpatialAudioEnginePrivate::bufferSize, d->sampleRate);
}

QSpatialAudioEngine::~QSpatialAudioEngine()
{
    stop();
    delete d;
}

void QSpatialAudioEngine::setOutputMode(OutputMode mode)
{
    if (d->outputMode == mode)
        return;
    d->outputMode = mode;
    if (d->api) {
        d->api->SetStereoSpeakerMode(mode == Stereo);
    }
    emit outputModeChanged();
}

QSpatialAudioEngine::OutputMode QSpatialAudioEngine::outputMode() const
{
    return d->outputMode;
}

int QSpatialAudioEngine::sampleRate() const
{
    return d->sampleRate;
}

void QSpatialAudioEngine::setOutputDevice(const QAudioDevice &device)
{
    if (d->device == device)
        return;
    if (d->api) {
        qWarning() << "Changing device on a running engine not implemented";
        return;
    }
    d->device = device;
    emit outputDeviceChanged();
}

QAudioDevice QSpatialAudioEngine::outputDevice() const
{
    return d->device;
}

void QSpatialAudioEngine::setMasterVolume(float volume)
{
    if (d->masterVolume == volume)
        return;
    d->masterVolume = volume;
    d->api->SetMasterVolume(volume);
    emit masterVolumeChanged();
}

float QSpatialAudioEngine::masterVolume() const
{
    return d->masterVolume;
}

void QSpatialAudioEngine::start()
{
    if (d->outputStream)
        // already started
        return;

    d->format.setChannelCount(2);
    d->format.setSampleRate(d->sampleRate);
    d->format.setSampleFormat(QAudioFormat::Float);

    d->api->SetStereoSpeakerMode(d->outputMode == Stereo);
    d->api->EnableRoomEffects(true);

    d->outputStream.reset(new QAudioOutputStream(d));
    d->outputStream->moveToThread(&d->audioThread);
    d->audioThread.start();

    QMetaObject::invokeMethod(d->outputStream.get(), "startOutput");
}

void QSpatialAudioEngine::stop()
{
    QMetaObject::invokeMethod(d->outputStream.get(), "stopOutput", Qt::BlockingQueuedConnection);
    d->outputStream.reset();
    d->audioThread.exit(0);
    d->audioThread.wait();
    delete d->api;
    d->api = nullptr;
}

void QSpatialAudioEngine::setRoomEffectsEnabled(bool enabled)
{
    if (d->roomEffectsEnabled == enabled)
        return;
    d->roomEffectsEnabled = enabled;
    d->api->EnableRoomEffects(d->roomEffectsEnabled);
}

bool QSpatialAudioEngine::roomEffectsEnabled() const
{
    return d->roomEffectsEnabled;
}

QT_END_NAMESPACE

#include "moc_qspatialaudioengine.cpp"
#include "qspatialaudioengine.moc"
