/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgstreameraudiodecodersession.h"
#include <private/qgstreamerbushelper_p.h>

#include <private/qgstutils_p.h>

#include <gst/gstvalue.h>
#include <gst/base/gstbasesrc.h>

#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qsize.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qstandardpaths.h>

QT_BEGIN_NAMESPACE

QGstreamerAudioDecoderSession::QGstreamerAudioDecoderSession(QObject *parent)
    : QObject(parent),
     m_state(QAudioDecoder::StoppedState),
     m_pendingState(QAudioDecoder::StoppedState),
     m_busHelper(0),
     m_bus(0),
     m_playbin(0),
#if defined(HAVE_GST_APPSRC)
     m_appSrc(0),
#endif
     mDevice(0)
{
    // Default format
    mFormat.setChannels(2);
    mFormat.setSampleSize(16);
    mFormat.setFrequency(48000);
    mFormat.setCodec("audio/x-raw");
    mFormat.setSampleType(QAudioFormat::UnSignedInt);


    // Create pipeline here
#if 0
    if (m_playbin != 0) {
        // Sort out messages
        m_bus = gst_element_get_bus(m_playbin);
        m_busHelper = new QGstreamerBusHelper(m_bus, this);
        m_busHelper->installMessageFilter(this);
    }
#endif
}

QGstreamerAudioDecoderSession::~QGstreamerAudioDecoderSession()
{
    if (m_playbin) {
        stop();

        delete m_busHelper;
        gst_object_unref(GST_OBJECT(m_bus));
        gst_object_unref(GST_OBJECT(m_playbin));
    }
}

#if defined(HAVE_GST_APPSRC)
void QGstreamerAudioDecoderSession::configureAppSrcElement(GObject* object, GObject *orig, GParamSpec *pspec, QGstreamerAudioDecoderSession* self)
{
    Q_UNUSED(object);
    Q_UNUSED(pspec);

    if (self->appsrc()->isReady())
        return;

    GstElement *appsrc;
    g_object_get(orig, "source", &appsrc, NULL);

    if (!self->appsrc()->setup(appsrc))
        qWarning()<<"Could not setup appsrc element";
}
#endif

#if 0
void QGstreamerAudioDecoderSession::loadFromStream(const QNetworkRequest &request, QIODevice *appSrcStream)
{
#if defined(HAVE_GST_APPSRC)
#ifdef DEBUG_PLAYBIN
    qDebug() << Q_FUNC_INFO;
#endif
    m_request = request;
    m_duration = -1;
    m_lastPosition = 0;
    m_haveQueueElement = false;

    if (m_appSrc)
        m_appSrc->deleteLater();
    m_appSrc = new QGstAppSrc(this);
    m_appSrc->setStream(appSrcStream);

    if (m_playbin) {
        m_tags.clear();
        emit tagsChanged();

        g_signal_connect(G_OBJECT(m_playbin), "deep-notify::source", (GCallback) &QGstreamerAudioDecoderSession::configureAppSrcElement, (gpointer)this);
        g_object_set(G_OBJECT(m_playbin), "uri", "appsrc://", NULL);

        if (!m_streamTypes.isEmpty()) {
            m_streamProperties.clear();
            m_streamTypes.clear();

            emit streamsChanged();
        }
    }
#endif
}

void QGstreamerAudioDecoderSession::loadFromUri(const QNetworkRequest &request)
{
#ifdef DEBUG_PLAYBIN
    qDebug() << Q_FUNC_INFO << request.url();
#endif
    m_request = request;
    m_duration = -1;
    m_lastPosition = 0;
    m_haveQueueElement = false;

    if (m_playbin) {
        m_tags.clear();
        emit tagsChanged();

        g_object_set(G_OBJECT(m_playbin), "uri", m_request.url().toEncoded().constData(), NULL);

        if (!m_streamTypes.isEmpty()) {
            m_streamProperties.clear();
            m_streamTypes.clear();

            emit streamsChanged();
        }
    }
}
#endif

bool QGstreamerAudioDecoderSession::processBusMessage(const QGstreamerMessage &message)
{
    GstMessage* gm = message.rawMessage();
    if (gm) {
        if (GST_MESSAGE_SRC(gm) == GST_OBJECT_CAST(m_playbin)) {
            switch (GST_MESSAGE_TYPE(gm))  {
            case GST_MESSAGE_STATE_CHANGED:
                {
                    GstState    oldState;
                    GstState    newState;
                    GstState    pending;

                    gst_message_parse_state_changed(gm, &oldState, &newState, &pending);

#ifdef DEBUG_PLAYBIN
                    QStringList states;
                    states << "GST_STATE_VOID_PENDING" <<  "GST_STATE_NULL" << "GST_STATE_READY" << "GST_STATE_PAUSED" << "GST_STATE_PLAYING";

                    qDebug() << QString("state changed: old: %1  new: %2  pending: %3") \
                            .arg(states[oldState]) \
                            .arg(states[newState]) \
                            .arg(states[pending]);
#endif

                    switch (newState) {
                    case GST_STATE_VOID_PENDING:
                    case GST_STATE_NULL:
                        if (m_state != QAudioDecoder::StoppedState)
                            emit stateChanged(m_state = QAudioDecoder::StoppedState);
                        break;
                    case GST_STATE_READY:
                        if (m_state != QAudioDecoder::StoppedState)
                            emit stateChanged(m_state = QAudioDecoder::StoppedState);
                        break;
                    case GST_STATE_PLAYING:
                        if (m_state != QAudioDecoder::DecodingState)
                            emit stateChanged(m_state = QAudioDecoder::DecodingState);

                        break;
                    case GST_STATE_PAUSED:
                        if (m_state != QAudioDecoder::WaitingState)
                            emit stateChanged(m_state = QAudioDecoder::WaitingState);
                        break;
                    }
                }
                break;

            case GST_MESSAGE_EOS:
                emit stateChanged(m_state = QAudioDecoder::StoppedState);
                break;

            case GST_MESSAGE_TAG:
            case GST_MESSAGE_STREAM_STATUS:
            case GST_MESSAGE_UNKNOWN:
                break;
            case GST_MESSAGE_ERROR: {
                    GError *err;
                    gchar *debug;
                    gst_message_parse_error(gm, &err, &debug);
                    if (err->domain == GST_STREAM_ERROR && err->code == GST_STREAM_ERROR_CODEC_NOT_FOUND)
                        processInvalidMedia(QAudioDecoder::FormatError, tr("Cannot play stream of type: <unknown>"));
                    else
                        processInvalidMedia(QAudioDecoder::ResourceError, QString::fromUtf8(err->message));
                    qWarning() << "Error:" << QString::fromUtf8(err->message);
                    g_error_free(err);
                    g_free(debug);
                }
                break;
            case GST_MESSAGE_WARNING:
                {
                    GError *err;
                    gchar *debug;
                    gst_message_parse_warning (gm, &err, &debug);
                    qWarning() << "Warning:" << QString::fromUtf8(err->message);
                    g_error_free (err);
                    g_free (debug);
                }
                break;
            case GST_MESSAGE_INFO:
#ifdef DEBUG_PLAYBIN
                {
                    GError *err;
                    gchar *debug;
                    gst_message_parse_info (gm, &err, &debug);
                    qDebug() << "Info:" << QString::fromUtf8(err->message);
                    g_error_free (err);
                    g_free (debug);
                }
#endif
                break;
            case GST_MESSAGE_BUFFERING:
            case GST_MESSAGE_STATE_DIRTY:
            case GST_MESSAGE_STEP_DONE:
            case GST_MESSAGE_CLOCK_PROVIDE:
            case GST_MESSAGE_CLOCK_LOST:
            case GST_MESSAGE_NEW_CLOCK:
            case GST_MESSAGE_STRUCTURE_CHANGE:
            case GST_MESSAGE_APPLICATION:
            case GST_MESSAGE_ELEMENT:
                break;
            case GST_MESSAGE_SEGMENT_START:
            case GST_MESSAGE_SEGMENT_DONE:
                break;
            case GST_MESSAGE_LATENCY:
#if (GST_VERSION_MAJOR >= 0) &&  (GST_VERSION_MINOR >= 10) && (GST_VERSION_MICRO >= 13)
            case GST_MESSAGE_ASYNC_START:
            case GST_MESSAGE_ASYNC_DONE:
#if GST_VERSION_MICRO >= 23
            case GST_MESSAGE_REQUEST_STATE:
#endif
#endif
            case GST_MESSAGE_ANY:
                break;
            default:
                break;
            }
        } else if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_ERROR) {
            GError *err;
            gchar *debug;
            gst_message_parse_error(gm, &err, &debug);
            // If the source has given up, so do we.
            if (qstrcmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(gm)), "source") == 0) {
                processInvalidMedia(QAudioDecoder::ResourceError, QString::fromUtf8(err->message));
            } else if (err->domain == GST_STREAM_ERROR
                       && (err->code == GST_STREAM_ERROR_DECRYPT || err->code == GST_STREAM_ERROR_DECRYPT_NOKEY)) {
                processInvalidMedia(QAudioDecoder::AccessDeniedError, QString::fromUtf8(err->message));
            }
            g_error_free(err);
            g_free(debug);
        }
    }

    return false;
}

QString QGstreamerAudioDecoderSession::sourceFilename() const
{
    return mSource;
}

void QGstreamerAudioDecoderSession::setSourceFilename(const QString &fileName)
{
    stop();
    mDevice = 0;
    mSource = fileName;
}

QIODevice *QGstreamerAudioDecoderSession::sourceDevice() const
{
    return mDevice;
}

void QGstreamerAudioDecoderSession::setSourceDevice(QIODevice *device)
{
    stop();
    mSource.clear();
    mDevice = device;
}

void QGstreamerAudioDecoderSession::start()
{
    // TODO
}

void QGstreamerAudioDecoderSession::stop()
{
    // TODO
}

QAudioFormat QGstreamerAudioDecoderSession::audioFormat() const
{
    return mFormat;
}

void QGstreamerAudioDecoderSession::setAudioFormat(const QAudioFormat &format)
{
    if (mFormat != format) {
        mFormat = format;
        emit formatChanged(mFormat);
    }
}

QAudioBuffer QGstreamerAudioDecoderSession::read(bool *ok)
{
    // TODO
    if (ok)
        *ok = false;
    return QAudioBuffer();
}

bool QGstreamerAudioDecoderSession::bufferAvailable() const
{
    return false;
}

void QGstreamerAudioDecoderSession::processInvalidMedia(QAudioDecoder::Error errorCode, const QString& errorString)
{
    stop();
    emit error(int(errorCode), errorString);
}

QT_END_NAMESPACE
