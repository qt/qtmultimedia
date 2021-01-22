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

#ifndef QAUDIODECODERCONTROL_H
#define QAUDIODECODERCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qaudiodecoder.h>

#include <QtCore/qpair.h>

#include <QtMultimedia/qaudiobuffer.h>

QT_BEGIN_NAMESPACE

class QIODevice;
class Q_MULTIMEDIA_EXPORT QAudioDecoderControl : public QMediaControl
{
    Q_OBJECT

public:
    ~QAudioDecoderControl();

    virtual QAudioDecoder::State state() const = 0;

    virtual QString sourceFilename() const = 0;
    virtual void setSourceFilename(const QString &fileName) = 0;

    virtual QIODevice* sourceDevice() const = 0;
    virtual void setSourceDevice(QIODevice *device) = 0;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual QAudioFormat audioFormat() const = 0;
    virtual void setAudioFormat(const QAudioFormat &format) = 0;

    virtual QAudioBuffer read() = 0;
    virtual bool bufferAvailable() const = 0;

    virtual qint64 position() const = 0;
    virtual qint64 duration() const = 0;

Q_SIGNALS:
    void stateChanged(QAudioDecoder::State newState);
    void formatChanged(const QAudioFormat &format);
    void sourceChanged();

    void error(int error, const QString &errorString);

    void bufferReady();
    void bufferAvailableChanged(bool available);
    void finished();

    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);

protected:
    explicit QAudioDecoderControl(QObject *parent = nullptr);
};

#define QAudioDecoderControl_iid "org.qt-project.qt.audiodecodercontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QAudioDecoderControl, QAudioDecoderControl_iid)

QT_END_NAMESPACE

#endif  // QAUDIODECODERCONTROL_H
