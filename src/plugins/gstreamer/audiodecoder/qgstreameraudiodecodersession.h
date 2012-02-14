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

#ifndef QGSTREAMERPLAYERSESSION_H
#define QGSTREAMERPLAYERSESSION_H

#include <QObject>
#include "qgstreameraudiodecodercontrol.h"
#include <private/qgstreamerbushelper_p.h>
#include <private/qaudiodecoder_p.h>

#if defined(HAVE_GST_APPSRC)
#include "qgstappsrc.h"
#endif

#include <gst/gst.h>

QT_BEGIN_NAMESPACE

class QGstreamerBusHelper;
class QGstreamerMessage;

class QGstreamerAudioDecoderSession : public QObject,
                                public QGstreamerBusMessageFilter
{
Q_OBJECT
Q_INTERFACES(QGstreamerBusMessageFilter)

public:
    QGstreamerAudioDecoderSession(QObject *parent);
    virtual ~QGstreamerAudioDecoderSession();

    QGstreamerBusHelper *bus() const { return m_busHelper; }

    QAudioDecoder::State state() const { return m_state; }
    QAudioDecoder::State pendingState() const { return m_pendingState; }

    bool processBusMessage(const QGstreamerMessage &message);

#if defined(HAVE_GST_APPSRC)
    QGstAppSrc *appsrc() const { return m_appSrc; }
    static void configureAppSrcElement(GObject*, GObject*, GParamSpec*,QGstreamerAudioDecoderSession* _this);
#endif

    QString sourceFilename() const;
    void setSourceFilename(const QString &fileName);

    QIODevice* sourceDevice() const;
    void setSourceDevice(QIODevice *device);

    void start();
    void stop();

    QAudioFormat audioFormat() const;
    void setAudioFormat(const QAudioFormat &format);

    QAudioBuffer read(bool *ok);
    bool bufferAvailable() const;

signals:
    void stateChanged(QAudioDecoder::State newState);
    void formatChanged(const QAudioFormat &format);

    void error(int error, const QString &errorString);

    void bufferReady();
    void bufferAvailableChanged(bool available);

private:

    void processInvalidMedia(QAudioDecoder::Error errorCode, const QString& errorString);

    QAudioDecoder::State m_state;
    QAudioDecoder::State m_pendingState;
    QGstreamerBusHelper* m_busHelper;
    GstBus* m_bus;
    GstElement* m_playbin;

#if defined(HAVE_GST_APPSRC)
    QGstAppSrc *m_appSrc;
#endif

    QString mSource;
    QIODevice *mDevice; // QWeakPointer perhaps
    QAudioFormat mFormat;
};

QT_END_NAMESPACE

#endif // QGSTREAMERPLAYERSESSION_H
