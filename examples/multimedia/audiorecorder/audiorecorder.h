// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QMainWindow>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QUrl>

QT_BEGIN_NAMESPACE
namespace Ui {
class AudioRecorder;
}
class QAudioBuffer;
QT_END_NAMESPACE

class AudioLevel;

class AudioRecorder : public QMainWindow
{
    Q_OBJECT

public:
    AudioRecorder();

public slots:
    void processBuffer(const QAudioBuffer &);

private slots:
    void init();
    void setOutputLocation();
    void togglePause();
    void toggleRecord();

    void onStateChanged(QMediaRecorder::RecorderState);
    void updateProgress(qint64 pos);
    void displayErrorMessage();

    void updateFormats();

private:
    void clearAudioLevels();
    QMediaFormat selectedMediaFormat() const;

    Ui::AudioRecorder *ui = nullptr;

    QMediaCaptureSession m_captureSession;
    QMediaRecorder *m_audioRecorder = nullptr;
    QList<AudioLevel *> m_audioLevels;
    bool m_outputLocationSet = false;
    bool m_updatingFormats = false;
};

#endif // AUDIORECORDER_H
