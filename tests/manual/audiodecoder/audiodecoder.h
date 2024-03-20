// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include <QAudioDecoder>
#include <QSoundEffect>
#include <QTextStream>
#include <QWaveDecoder>

class AudioDecoder : public QObject
{
    Q_OBJECT

public:
    AudioDecoder(bool isPlayback, bool isDelete, const QString &targetFileName);
    ~AudioDecoder();

    void setSource(const QString &fileName);
    void start();
    void stop();
    QAudioDecoder::Error getError();

    void setTargetFilename(const QString &fileName);

signals:
    void done();

public slots:
    void bufferReady();
    void error(QAudioDecoder::Error error);
    void isDecodingChanged(bool isDecoding);
    void finished();

    void playbackStatusChanged();
    void playingChanged();

private slots:
    void updateProgress();

private:
    bool m_isPlayback;
    bool m_isDelete;
    QAudioDecoder m_decoder;
    QTextStream m_cout;

    QString m_targetFilename;
    QWaveDecoder *m_waveDecoder = nullptr;
    QSoundEffect m_soundEffect;

    qreal m_progress;
};

#endif // AUDIODECODER_H
