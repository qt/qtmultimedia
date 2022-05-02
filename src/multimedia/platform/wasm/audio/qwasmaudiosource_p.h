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

#ifndef QWASMAUDIOSOURCE_H
#define QWASMAUDIOSOURCE_H

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

#include <private/qaudiosystem_p.h>
#include <QTimer>
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE

class ALData;

class QWasmAudioSource : public QPlatformAudioSource
{
    Q_OBJECT

    QByteArray m_name;
    ALData *aldata = nullptr;
    QTimer m_timer;
    QIODevice *m_device = nullptr;
    QAudioFormat m_format;
    qreal m_volume = 1;
    int m_bufferSize;
    bool m_running = false;
    bool m_suspended = false;
    QAudio::Error m_error;
    bool m_pullMode;
    char *m_tmpData = nullptr;
    QElapsedTimer m_elapsedTimer;
    int m_notifyInterval = 0;
    quint64 m_processed = 0;

    void writeBuffer();
public:
    QWasmAudioSource(const QByteArray &device);

public:
    void start(QIODevice *device) override;
    QIODevice *start() override;
    void start(bool mode);
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    int bytesReady() const override;
    void setBufferSize(int value) override;
    int bufferSize() const override;
    qint64 processedUSecs() const override;
    QAudio::Error error() const override;
    QAudio::State state() const override;
    void setFormat(const QAudioFormat &fmt) override;
    QAudioFormat format() const override;
    void setVolume(qreal volume) override;
    qreal volume() const override;

    friend class QWasmAudioSourceDevice;
    void setError(const QAudio::Error &error);
};

QT_END_NAMESPACE

#endif // QWASMAUDIOSOURCE_H
