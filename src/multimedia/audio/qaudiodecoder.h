/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
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
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QAUDIODECODER_H
#define QAUDIODECODER_H

#include <QtCore/qobject.h>
#include <QtMultimedia/qmediaenumdebug.h>

#include <QtMultimedia/qaudiobuffer.h>

QT_BEGIN_NAMESPACE

class QPlatformAudioDecoder;
class Q_MULTIMEDIA_EXPORT QAudioDecoder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool isDecoding READ isDecoding NOTIFY isDecodingChanged)
    Q_PROPERTY(QString error READ errorString)
    Q_PROPERTY(bool bufferAvailable READ bufferAvailable NOTIFY bufferAvailableChanged)

public:
    enum Error
    {
        NoError,
        ResourceError,
        FormatError,
        AccessDeniedError,
        NotSupportedError
    };
    Q_ENUM(Error)

    explicit QAudioDecoder(QObject *parent = nullptr);
    ~QAudioDecoder();

    bool isSupported() const;
    bool isDecoding() const;

    QUrl source() const;
    void setSource(const QUrl &fileName);

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
    void isDecodingChanged(bool);

    void formatChanged(const QAudioFormat &format);

    void error(QAudioDecoder::Error error);

    void sourceChanged();

    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);

private:
    Q_DISABLE_COPY(QAudioDecoder)
    QPlatformAudioDecoder *decoder;
};

QT_END_NAMESPACE

Q_MEDIA_ENUM_DEBUG(QAudioDecoder, Error)

#endif  // QAUDIODECODER_H
