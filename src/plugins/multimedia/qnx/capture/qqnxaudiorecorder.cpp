// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqnxaudiorecorder_p.h"
#include "qqnxmediaeventthread_p.h"

#include <QtCore/qcoreapplication.h>

#include <private/qmediastoragelocation_p.h>

#include <mm/renderer.h>

#include <sys/stat.h>
#include <sys/strm.h>

static QByteArray buildDevicePath(const QByteArray &deviceId, const QMediaEncoderSettings &settings)
{
    QByteArray devicePath = QByteArrayLiteral("snd:/dev/snd/") + deviceId + QByteArrayLiteral("?");

    if (settings.audioSampleRate() > 0)
        devicePath += QByteArrayLiteral("frate=") + QByteArray::number(settings.audioSampleRate());

    if (settings.audioChannelCount() > 0)
        devicePath += QByteArrayLiteral("nchan=") + QByteArray::number(settings.audioChannelCount());

    return devicePath;
}

QT_BEGIN_NAMESPACE

QQnxAudioRecorder::QQnxAudioRecorder(QObject *parent)
    : QObject(parent)
{
    openConnection();
}

QQnxAudioRecorder::~QQnxAudioRecorder()
{
    stop();
    closeConnection();
}

void QQnxAudioRecorder::openConnection()
{
    static int idCounter = 0;

    m_connection = ConnectionUniquePtr { mmr_connect(nullptr) };

    if (!m_connection) {
        qWarning("QQnxAudioRecorder: Unable to connect to the multimedia renderer");
        return;
    }

    m_id = idCounter++;

    char contextName[256];

    std::snprintf(contextName, sizeof contextName, "QQnxAudioRecorder_%d_%llu",
            m_id, QCoreApplication::applicationPid());

    m_context = ContextUniquePtr { mmr_context_create(m_connection.get(),
            contextName, 0, S_IRWXU|S_IRWXG|S_IRWXO) };

    if (m_context) {
        startMonitoring();
    } else {
        qWarning("QQnxAudioRecorder: Unable to create context");
        closeConnection();
    }
}

void QQnxAudioRecorder::closeConnection()
{
    m_context.reset();
    m_context.reset();

    stopMonitoring();
}

void QQnxAudioRecorder::attach()
{
    if (isAttached())
        return;

    const QString container = m_encoderSettings.preferredSuffix();
    const QString location = QMediaStorageLocation::generateFileName(m_outputUrl.toLocalFile(),
            QStandardPaths::MusicLocation, container);

    m_audioId = mmr_output_attach(m_context.get(), qPrintable(location), "file");

    if (m_audioId == -1) {
        qWarning("QQnxAudioRecorder: mmr_output_attach() for file failed");
        return;
    }

    configureOutputBitRate();

    const QByteArray devicePath = buildDevicePath(m_inputDeviceId, m_encoderSettings);

    if (mmr_input_attach(m_context.get(), devicePath.constData(), "track") != 0) {
        qWarning("QQnxAudioRecorder: mmr_input_attach() failed");
        detach();
    } else {
        Q_EMIT actualLocationChanged(location);
    }
}

void QQnxAudioRecorder::detach()
{
    if (!isAttached())
        return;

    mmr_input_detach(m_context.get());
    mmr_output_detach(m_context.get(), m_audioId);

    m_audioId = -1;
}

void QQnxAudioRecorder::configureOutputBitRate()
{
    const int bitRate = m_encoderSettings.audioBitRate();

    if (!isAttached() || bitRate <= 0)
        return;

    char buf[12];
    std::snprintf(buf, sizeof buf, "%d", bitRate);

    strm_dict_t *dict = strm_dict_new();
    dict = strm_dict_set(dict, "audio_bitrate", buf);

    if (mmr_output_parameters(m_context.get(), m_audioId, dict) != 0)
        qWarning("mmr_output_parameters: setting bitrate failed");
}

bool QQnxAudioRecorder::isAttached() const
{
    return m_context && m_audioId != -1;
}

void QQnxAudioRecorder::setInputDeviceId(const QByteArray &id)
{
    m_inputDeviceId = id;
}

void QQnxAudioRecorder::setOutputUrl(const QUrl &url)
{
    m_outputUrl = url;
}

void QQnxAudioRecorder::setMediaEncoderSettings(const QMediaEncoderSettings &settings)
{
    m_encoderSettings = settings;
}

void QQnxAudioRecorder::record()
{
    if (!isAttached()) {
        attach();

        if (!isAttached())
            return;
    }

    if (mmr_play(m_context.get()) != 0)
        qWarning("QQnxAudioRecorder: mmr_play() failed");
}

void QQnxAudioRecorder::stop()
{
    if (!isAttached())
        return;

    mmr_stop(m_context.get());

    detach();
}

void QQnxAudioRecorder::startMonitoring()
{
    m_eventThread = std::make_unique<QQnxMediaEventThread>(m_context.get());

    connect(m_eventThread.get(), &QQnxMediaEventThread::eventPending,
            this, &QQnxAudioRecorder::readEvents);

    m_eventThread->setObjectName(QStringLiteral("MmrAudioEventThread-") + QString::number(m_id));
    m_eventThread->start();
}

void QQnxAudioRecorder::stopMonitoring()
{
    if (m_eventThread)
        m_eventThread.reset();
}

void QQnxAudioRecorder::readEvents()
{
    while (const mmr_event_t *event = mmr_event_get(m_context.get())) {
        if (event->type == MMR_EVENT_NONE)
            break;

        switch (event->type) {
        case MMR_EVENT_STATUS:
            handleMmEventStatus(event);
            break;
        case MMR_EVENT_STATE:
            handleMmEventState(event);
            break;
        case MMR_EVENT_ERROR:
            handleMmEventError(event);
            break;
        case MMR_EVENT_METADATA:
        case MMR_EVENT_NONE:
        case MMR_EVENT_OVERFLOW:
        case MMR_EVENT_WARNING:
        case MMR_EVENT_PLAYLIST:
        case MMR_EVENT_INPUT:
        case MMR_EVENT_OUTPUT:
        case MMR_EVENT_CTXTPAR:
        case MMR_EVENT_TRKPAR:
        case MMR_EVENT_OTHER:
            break;
        }
    }

    if (m_eventThread)
        m_eventThread->signalRead();
}

void QQnxAudioRecorder::handleMmEventStatus(const mmr_event_t *event)
{
    if (!event || event->type != MMR_EVENT_STATUS)
        return;

    if (!event->pos_str)
        return;

    const QByteArray valueBa(event->pos_str);

    bool ok;
    const qint64 duration = valueBa.toLongLong(&ok);

    if (!ok)
        qCritical("Could not parse duration from '%s'", valueBa.constData());
    else
        durationChanged(duration);
}

void QQnxAudioRecorder::handleMmEventState(const mmr_event_t *event)
{
    if (!event || event->type != MMR_EVENT_STATE)
        return;

    switch (event->state) {
    case MMR_STATE_DESTROYED:
    case MMR_STATE_IDLE:
    case MMR_STATE_STOPPED:
        Q_EMIT stateChanged(QMediaRecorder::StoppedState);
        break;
    case MMR_STATE_PLAYING:
        Q_EMIT stateChanged(QMediaRecorder::RecordingState);
        break;
    }
}

void QQnxAudioRecorder::handleMmEventError(const mmr_event_t *event)
{
    if (!event)
        return;

    // When playback is explicitly stopped using mmr_stop(), mm-renderer
    // generates a STATE event. When the end of media is reached, an ERROR
    // event is generated and the error code contained in the event information
    // is set to MMR_ERROR_NONE. When an error causes playback to stop,
    // the error code is set to something else.
    if (event->details.error.info.error_code == MMR_ERROR_NONE) {
        //TODO add error
        Q_EMIT stateChanged(QMediaRecorder::StoppedState);
        detach();
    }
}

QT_END_NAMESPACE

#include "moc_qqnxaudiorecorder_p.cpp"
