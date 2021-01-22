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

#ifndef QAUDIODECODER_H
#define QAUDIODECODER_H

#include <QtMultimedia/qmediaobject.h>
#include <QtMultimedia/qmediaenumdebug.h>

#include <QtMultimedia/qaudiobuffer.h>

QT_BEGIN_NAMESPACE

class QAudioDecoderPrivate;
class Q_MULTIMEDIA_EXPORT QAudioDecoder : public QMediaObject
{
    Q_OBJECT
    Q_PROPERTY(QString sourceFilename READ sourceFilename WRITE setSourceFilename NOTIFY sourceChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString error READ errorString)
    Q_PROPERTY(bool bufferAvailable READ bufferAvailable NOTIFY bufferAvailableChanged)

    Q_ENUMS(State)
    Q_ENUMS(Error)

public:
    enum State
    {
        StoppedState,
        DecodingState
    };

    enum Error
    {
        NoError,
        ResourceError,
        FormatError,
        AccessDeniedError,
        ServiceMissingError
    };

    explicit QAudioDecoder(QObject *parent = nullptr);
    ~QAudioDecoder();

    static QMultimedia::SupportEstimate hasSupport(const QString &mimeType, const QStringList& codecs = QStringList());

    State state() const;

    QString sourceFilename() const;
    void setSourceFilename(const QString &fileName);

    QIODevice* sourceDevice() const;
    void setSourceDevice(QIODevice *device);

    QAudioFormat audioFormat() const;
    void setAudioFormat(const QAudioFormat &format);

    Error error() const;
    QString errorString() const;

    QAudioBuffer read() const;
    bool bufferAvailable() const;

    qint64 position() const;
    qint64 duration() const;

public Q_SLOTS:
    void start();
    void stop();

Q_SIGNALS:
    void bufferAvailableChanged(bool);
    void bufferReady();
    void finished();

    void stateChanged(QAudioDecoder::State newState);
    void formatChanged(const QAudioFormat &format);

    void error(QAudioDecoder::Error error);

    void sourceChanged();

    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);

public:
    bool bind(QObject *) override;
    void unbind(QObject *) override;

private:
    Q_DISABLE_COPY(QAudioDecoder)
    Q_DECLARE_PRIVATE(QAudioDecoder)
    Q_PRIVATE_SLOT(d_func(), void _q_stateChanged(QAudioDecoder::State))
    Q_PRIVATE_SLOT(d_func(), void _q_error(int, const QString &))
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QAudioDecoder::State)
Q_DECLARE_METATYPE(QAudioDecoder::Error)

Q_MEDIA_ENUM_DEBUG(QAudioDecoder, State)
Q_MEDIA_ENUM_DEBUG(QAudioDecoder, Error)

#endif  // QAUDIODECODER_H
