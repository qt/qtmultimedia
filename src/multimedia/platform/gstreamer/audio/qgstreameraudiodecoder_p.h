/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QGSTREAMERAUDIODECODERCONTROL_H
#define QGSTREAMERAUDIODECODERCONTROL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QObject>
#include <QtCore/qmutex.h>
#include <QtCore/qurl.h>

#include "private/qplatformaudiodecoder_p.h"
#include <private/qgstpipeline_p.h>
#include "qaudiodecoder.h"

#if QT_CONFIG(gstreamer_app)
#include <private/qgstappsrc_p.h>
#endif

#include <private/qgst_p.h>
#include <gst/app/gstappsink.h>

QT_BEGIN_NAMESPACE

class QGstreamerMessage;

class QGstreamerAudioDecoder
        : public QPlatformAudioDecoder,
          public QGstreamerBusMessageFilter
{
    Q_OBJECT

public:
    QGstreamerAudioDecoder(QAudioDecoder *parent);
    virtual ~QGstreamerAudioDecoder();

    QUrl source() const override;
    void setSource(const QUrl &fileName) override;

    QIODevice *sourceDevice() const override;
    void setSourceDevice(QIODevice *device) override;

    void start() override;
    void stop() override;

    QAudioFormat audioFormat() const override;
    void setAudioFormat(const QAudioFormat &format) override;

    QAudioBuffer read() override;
    bool bufferAvailable() const override;

    qint64 position() const override;
    qint64 duration() const override;

    // GStreamerBusMessageFilter interface
    bool processBusMessage(const QGstreamerMessage &message) override;

#if QT_CONFIG(gstreamer_app)
    QGstAppSrc *appsrc() const { return m_appSrc; }
    static void configureAppSrcElement(GObject*, GObject*, GParamSpec*, QGstreamerAudioDecoder *_this);
#endif

    static GstFlowReturn new_sample(GstAppSink *sink, gpointer user_data);

private slots:
    void updateDuration();

private:
    void setAudioFlags(bool wantNativeAudio);
    void addAppSink();
    void removeAppSink();

    void processInvalidMedia(QAudioDecoder::Error errorCode, const QString& errorString);
    static qint64 getPositionFromBuffer(GstBuffer* buffer);

    QGstPipeline m_playbin;
    QGstBin m_outputBin;
    QGstElement m_audioConvert;
    GstAppSink *m_appSink = nullptr;
    QGstAppSrc *m_appSrc = nullptr;

    QUrl mSource;
    QIODevice *mDevice = nullptr;
    QAudioFormat mFormat;

    mutable QMutex m_buffersMutex;
    int m_buffersAvailable = 0;

    qint64 m_position = -1;
    qint64 m_duration = -1;

    int m_durationQueries = 0;
};

QT_END_NAMESPACE

#endif // QGSTREAMERPLAYERSESSION_H
