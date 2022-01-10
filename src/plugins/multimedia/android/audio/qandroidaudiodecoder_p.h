/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QANDROIDAUDIODECODER_P_H
#define QANDROIDAUDIODECODER_P_H

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
#include "private/qplatformaudiodecoder_p.h"

#include <QtCore/qurl.h>
#include <QtCore/qmutex.h>
#include <QThread>

#include "media/NdkMediaCodec.h"
#include "media/NdkMediaExtractor.h"
#include "media/NdkMediaFormat.h"
#include "media/NdkMediaError.h"


QT_USE_NAMESPACE

class Decoder : public QObject
{
    Q_OBJECT
public:
    Decoder();
    ~Decoder();

public slots:
    void setSource(const QUrl &source);
    void doDecode();
    void stop();

signals:
    void positionChanged(const QAudioBuffer &buffer, qint64 position);
    void durationChanged(const qint64 duration);
    void error(const QAudioDecoder::Error error, const QString &errorString);
    void finished();

private:
    void createDecoder();

    AMediaCodec *m_codec = nullptr;
    AMediaExtractor *m_extractor = nullptr;
    AMediaFormat *m_format = nullptr;

    QAudioFormat m_outputFormat;
    bool m_inputEOS;
};


class QAndroidAudioDecoder : public QPlatformAudioDecoder
{
    Q_OBJECT
public:
    QAndroidAudioDecoder(QAudioDecoder *parent);
        virtual ~QAndroidAudioDecoder();

    QUrl source() const override { return m_source; }
    void setSource(const QUrl &fileName) override;

    QIODevice *sourceDevice() const override { return m_device; }
    void setSourceDevice(QIODevice *device) override;

    void start() override;
    void stop() override;

    QAudioFormat audioFormat() const override { return {}; }
    void setAudioFormat(const QAudioFormat &/*format*/) override {}

    QAudioBuffer read() override;
    bool bufferAvailable() const override;

    qint64 position() const override;
    qint64 duration() const override;

private slots:
    void positionChanged(QAudioBuffer audioBuffer, qint64 position);
    void durationChanged(qint64 duration);
    void error(const QAudioDecoder::Error error, const QString &errorString);
    void readDevice();
    void finished();

private:
    bool requestPermissions();
    bool createTempFile();
    void decode();

    QIODevice *m_device = nullptr;
    Decoder *m_decoder;

    QList<QAudioBuffer> m_audioBuffer;
    QUrl m_source;

    mutable QMutex m_buffersMutex;
    qint64 m_position = -1;
    qint64 m_duration = -1;
    long long m_presentationTimeUs = 0;
    int m_buffersAvailable = 0;

    QByteArray m_deviceBuffer;

    QThread *m_threadDecoder;
};

QT_END_NAMESPACE

#endif // QANDROIDAUDIODECODER_P_H
