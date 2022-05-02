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

#ifndef QWASMAUDIOSINK_H
#define QWASMAUDIOSINK_H

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

class ALData;
class QIODevice;

QT_BEGIN_NAMESPACE

class QWasmAudioSink : public QPlatformAudioSink
{
    Q_OBJECT

    QByteArray m_name;
    ALData *aldata = nullptr;
    QTimer m_timer;
    QIODevice *m_device = nullptr;
    QAudioFormat m_format;
    QAudio::Error m_error = QAudio::NoError;
    bool m_running = false;
    QAudio::State m_state = QAudio::StoppedState;
    int m_bufferSize = 0;
    quint64 m_processed = 0;
    QElapsedTimer m_elapsedTimer;
    int m_bufferFragmentsCount = 10;
    int m_notifyInterval = 0;
    char *m_tmpData = nullptr;
    int m_bufferFragmentSize = 0;
    int m_lastNotified = 0;
    int m_tmpDataOffset = 0;
    int m_bufferFragmentsBusyCount = 0;
    bool m_pullMode;
    qreal m_volume = 1;

    void loadALBuffers();
    void unloadALBuffers();
    void nextALBuffers();

private slots:
    void updateState();
    void setError(QAudio::Error);

public:
    QWasmAudioSink(const QByteArray &device);
    ~QWasmAudioSink();

public:
    void start(QIODevice *device) override;
    QIODevice *start() override;
    void start(bool mode);
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    int bytesFree() const override;
    void setBufferSize(int value) override;
    int bufferSize() const override;
    qint64 processedUSecs() const override;
    QAudio::Error error() const override;
    QAudio::State state() const override;
    void setFormat(const QAudioFormat &fmt) override;
    QAudioFormat format() const override;
    void setVolume(qreal volume) override;
    qreal volume() const override;

    friend class QWasmAudioSinkDevice;
};

QT_END_NAMESPACE

#endif // QWASMAUDIOSINK_H
