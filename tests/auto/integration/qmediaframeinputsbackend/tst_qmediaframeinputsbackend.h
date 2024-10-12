// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QMEDIAFRAMEINPUTSBACKEND_H
#define TST_QMEDIAFRAMEINPUTSBACKEND_H

#include <QObject>

QT_BEGIN_NAMESPACE

class tst_QMediaFrameInputsBackend : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void mediaRecorderWritesAudio_whenAudioFramesInputSends_data();
    void mediaRecorderWritesAudio_whenAudioFramesInputSends();

    void mediaRecorderWritesVideo_whenVideoFramesInputSendsFrames_data();
    void mediaRecorderWritesVideo_whenVideoFramesInputSendsFrames();

    void mediaRecorderWritesVideo_withSingleFrame();

    void sinkReceivesFrameWithTransformParams_whenPresentationTransformPresent_data();
    void sinkReceivesFrameWithTransformParams_whenPresentationTransformPresent();

    void readyToSend_isEmitted_whenRecordingStarts_data();
    void readyToSend_isEmitted_whenRecordingStarts();

    void readyToSendVideoFrame_isEmitted_whenSendVideoFrameIsCalled();
    void readyToSendAudioBuffer_isEmitted_whenSendAudioBufferIsCalled();

    void readyToSendVideoFrame_isEmittedRepeatedly_whenPullModeIsEnabled();
    void readyToSendAudioBuffer_isEmittedRepeatedly_whenPullModeIsEnabled();
    void readyToSendAudioBufferAndVideoFrame_isEmittedRepeatedly_whenPullModeIsEnabled();

};

QT_END_NAMESPACE

#endif
