/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QAUDIOOUTPUTWASAPI_H
#define QAUDIOOUTPUTWASAPI_H

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QTime>
#include <QtMultimedia/QAbstractAudioOutput>
#include <QtMultimedia/QAudio>

#include <QReadWriteLock>
#include <wrl.h>

struct IAudioRenderClient;
struct IAudioStreamVolume;

QT_BEGIN_NAMESPACE

class AudioInterface;
class WasapiOutputDevicePrivate;
class QWasapiProcessThread;

Q_DECLARE_LOGGING_CATEGORY(lcMmAudioOutput)

class QWasapiAudioOutput : public QAbstractAudioOutput
{
    Q_OBJECT
public:
    explicit QWasapiAudioOutput(const QByteArray &device);
    ~QWasapiAudioOutput();

    void start(QIODevice* device) override;
    QIODevice* start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    int bytesFree() const override;
    int periodSize() const override;
    void setBufferSize(int value) override;
    int bufferSize() const override;
    void setNotifyInterval(int milliSeconds) override;
    int notifyInterval() const override;
    qint64 processedUSecs() const override;
    qint64 elapsedUSecs() const override;
    QAudio::Error error() const override;
    QAudio::State state() const override;
    void setFormat(const QAudioFormat& fmt) override;
    QAudioFormat format() const override;
    void setVolume(qreal) override;
    qreal volume() const override;

    void process();
public slots:
    void processBuffer();
private:
    bool initStart(bool pull);
    friend class WasapiOutputDevicePrivate;
    friend class WasapiOutputPrivate;
    QByteArray m_deviceName;
    Microsoft::WRL::ComPtr<AudioInterface> m_interface;
    Microsoft::WRL::ComPtr<IAudioRenderClient> m_renderer;
    Microsoft::WRL::ComPtr<IAudioStreamVolume> m_volumeControl;
    qreal m_volumeCache;
    QMutex m_mutex;
    QAudio::State m_currentState;
    QAudio::Error m_currentError;
    QAudioFormat m_currentFormat;
    qint64 m_bytesProcessed;
    QTime m_openTime;
    int m_openTimeOffset;
    int m_interval;
    bool m_pullMode;
    quint32 m_bufferFrames;
    quint32 m_bufferBytes;
    HANDLE m_event;
    QWasapiProcessThread *m_eventThread;
    QAtomicInt m_processing;
    QIODevice *m_eventDevice;
};

QT_END_NAMESPACE

#endif
