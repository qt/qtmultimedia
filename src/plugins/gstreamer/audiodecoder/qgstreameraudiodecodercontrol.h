/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QGSTREAMERPLAYERCONTROL_H
#define QGSTREAMERPLAYERCONTROL_H

#include <QtCore/qobject.h>
#include <QtCore/qstack.h>

#include <qaudioformat.h>
#include <qaudiobuffer.h>
#include <qaudiodecoder.h>
#include <qaudiodecodercontrol.h>

#include <limits.h>


QT_BEGIN_NAMESPACE

class QGstreamerAudioDecoderSession;
class QGstreamerAudioDecoderService;

class QGstreamerAudioDecoderControl : public QAudioDecoderControl
{
    Q_OBJECT

public:
    QGstreamerAudioDecoderControl(QGstreamerAudioDecoderSession *session, QObject *parent = 0);
    ~QGstreamerAudioDecoderControl();

    QAudioDecoder::State state() const override;

    QString sourceFilename() const override;
    void setSourceFilename(const QString &fileName) override;

    QIODevice* sourceDevice() const override;
    void setSourceDevice(QIODevice *device) override;

    void start() override;
    void stop() override;

    QAudioFormat audioFormat() const override;
    void setAudioFormat(const QAudioFormat &format) override;

    QAudioBuffer read() override;
    bool bufferAvailable() const override;

    qint64 position() const override;
    qint64 duration() const override;

private:
    // Stuff goes here

    QGstreamerAudioDecoderSession *m_session;
};

QT_END_NAMESPACE

#endif
