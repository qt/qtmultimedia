// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtGui/QImageReader>
#include <QtCore/qrandom.h>
#include <QtCore/qurl.h>
#include <QDebug>
#include <QVideoSink>
#include <QVideoWidget>
#include <QSysInfo>

#include <qcamera.h>
#include <qcameradevice.h>
#include <qimagecapture.h>
#include <qmediacapturesession.h>
#include <qmediaplayer.h>
#include <qmediadevices.h>
#include <qmediarecorder.h>
#include <qaudiooutput.h>
#include <qaudioinput.h>
#include <qaudiodevice.h>
#include <qaudiodecoder.h>
#include <qaudiobuffer.h>
#include <qscreencapture.h>
#include <qwindowcapture.h>
#include <qaudiobufferinput.h>
#include <qvideoframeinput.h>

#include <qcamera.h>
#include <QMediaFormat>
#include <QtMultimediaWidgets/QVideoWidget>

#include <private/mediabackendutils_p.h>

QT_USE_NAMESPACE

// NOLINTBEGIN(readability-convert-member-functions-to-static)

/*
 This is the backend conformance test.

 Since it relies on platform media framework and sound hardware
 it may be less stable.
*/

class tst_QMediaCaptureSession: public QObject
{
    Q_OBJECT

private slots:

    void testAudioMute();
    void stress_test_setup_and_teardown();
    void stress_test_setup_and_teardown_keep_session();
    void stress_test_setup_and_teardown_keep_recorder();
    void stress_test_setup_and_teardown_keep_camera();
    void stress_test_setup_and_teardown_keep_audioinput();
    void stress_test_setup_and_teardown_keep_audiooutput();
    void stress_test_setup_and_teardown_keep_video();

    void record_video_without_preview();

    void can_add_and_remove_AudioInput_with_and_without_AudioOutput_attached();
    void can_change_AudioDevices_on_attached_AudioInput();
    void can_change_AudioInput_during_recording();
    void disconnects_deleted_AudioInput();
    void can_move_AudioInput_between_sessions();

    void disconnects_deleted_AudioOutput();
    void can_move_AudioOutput_between_sessions_and_player();

    void disconnects_deleted_AudioBufferInput();
    void can_move_AudioBufferInput_between_sessions();

    void disconnects_deleted_VideoFrameInput();
    void can_move_VideoFrameInput_between_sessions();

    void can_add_and_remove_Camera();
    void can_move_Camera_between_sessions();
    void can_disconnect_Camera_when_recording();
    void can_add_and_remove_different_Cameras();
    void can_change_CameraDevice_on_attached_Camera();

    void can_change_VideoOutput_with_and_without_camera();
    void can_change_VideoOutput_when_recording();

    void can_add_and_remove_recorders();
    void can_move_Recorder_between_sessions();
    void cannot_record_without_Camera_and_AudioInput();
    void can_record_AudioInput_with_null_AudioDevice();
    void can_record_Camera_with_null_CameraDevice();
    void recording_stops_when_recorder_removed();

    void can_add_and_remove_ImageCapture();
    void can_move_ImageCapture_between_sessions();
    void capture_is_not_available_when_Camera_is_null();
    void can_add_ImageCapture_and_capture_during_recording();

    void can_switch_audio_output();
    void can_switch_audio_input();

    void can_clear_audio_output();
    void can_clear_audio_input();

private:
    void recordOk(QMediaCaptureSession &session);
    void recordFail(QMediaCaptureSession &session);
};


void tst_QMediaCaptureSession::recordOk(QMediaCaptureSession &session)
{
    QMediaRecorder recorder;
    session.setRecorder(&recorder);

    QSignalSpy recorderErrorSignal(&recorder, &QMediaRecorder::errorOccurred);
    QSignalSpy durationChanged(&recorder, &QMediaRecorder::durationChanged);

    recorder.record();
    QTRY_VERIFY_WITH_TIMEOUT(recorder.recorderState() == QMediaRecorder::RecordingState, 2000);
    QVERIFY(durationChanged.wait(2000));
    recorder.stop();

    QTRY_VERIFY_WITH_TIMEOUT(recorder.recorderState() == QMediaRecorder::StoppedState, 2000);
    QVERIFY(recorderErrorSignal.isEmpty());

    QString fileName = recorder.actualLocation().toLocalFile();
    QVERIFY(!fileName.isEmpty());
    QTRY_VERIFY(QFileInfo(fileName).size() > 0);
    QFile(fileName).remove();
}

void tst_QMediaCaptureSession::recordFail(QMediaCaptureSession &session)
{
    QMediaRecorder recorder;
    QSignalSpy recorderErrorSignal(&recorder, &QMediaRecorder::errorOccurred);

    session.setRecorder(&recorder);
    recorder.record();

    QTRY_VERIFY_WITH_TIMEOUT(recorderErrorSignal.size() == 1, 2000);
    QTRY_VERIFY_WITH_TIMEOUT(recorder.recorderState() == QMediaRecorder::StoppedState, 2000);
}

void tst_QMediaCaptureSession::stress_test_setup_and_teardown()
{
    QSKIP_GSTREAMER("QTBUG-124005: spurious seek failures on CI");

    for (int i = 0; i < 50; i++) {
        QMediaCaptureSession session;
        QMediaRecorder recorder;
        QCamera camera;
        QAudioInput input;
        QAudioOutput output;
        QVideoWidget video;

        if (!input.device().isNull())
            session.setAudioInput(&input);
        if (!output.device().isNull())
            session.setAudioOutput(&output);
        session.setRecorder(&recorder);
        session.setCamera(&camera);
        session.setVideoOutput(&video);

        QRandomGenerator rng;
        QTest::qWait(rng.bounded(200));
    }
}

void tst_QMediaCaptureSession::stress_test_setup_and_teardown_keep_session()
{
    QSKIP_GSTREAMER("QTBUG-124005: spurious seek failures on CI");

    QMediaCaptureSession session;
    for (int i = 0; i < 50; i++) {
        QMediaRecorder recorder;
        QCamera camera;
        QAudioInput input;
        QAudioOutput output;
        QVideoWidget video;

        if (!input.device().isNull())
            session.setAudioInput(&input);
        if (!output.device().isNull())
            session.setAudioOutput(&output);
        session.setRecorder(&recorder);
        session.setCamera(&camera);
        session.setVideoOutput(&video);

        QRandomGenerator rng;
        QTest::qWait(rng.bounded(200));
    }
}

void tst_QMediaCaptureSession::stress_test_setup_and_teardown_keep_recorder()
{
    QSKIP_GSTREAMER("QTBUG-124005: spurious seek failures on CI");

    QMediaCaptureSession session;
    QMediaRecorder recorder;
    for (int i = 0; i < 50; i++) {
        QCamera camera;
        QAudioInput input;
        QAudioOutput output;
        QVideoWidget video;

        if (!input.device().isNull())
            session.setAudioInput(&input);
        if (!output.device().isNull())
            session.setAudioOutput(&output);
        session.setRecorder(&recorder);
        session.setCamera(&camera);
        session.setVideoOutput(&video);

        QRandomGenerator rng;
        QTest::qWait(rng.bounded(200));
    }
}

void tst_QMediaCaptureSession::stress_test_setup_and_teardown_keep_camera()
{
    QSKIP_GSTREAMER("QTBUG-124005: spurious seek failures on CI");

    QCamera camera;
    for (int i = 0; i < 50; i++) {
        QMediaCaptureSession session;
        QMediaRecorder recorder;
        QAudioInput input;
        QAudioOutput output;
        QVideoWidget video;

        if (!input.device().isNull())
            session.setAudioInput(&input);
        if (!output.device().isNull())
            session.setAudioOutput(&output);
        session.setRecorder(&recorder);
        session.setCamera(&camera);
        session.setVideoOutput(&video);

        QRandomGenerator rng;
        QTest::qWait(rng.bounded(200));
    }
}

void tst_QMediaCaptureSession::stress_test_setup_and_teardown_keep_audioinput()
{
    QSKIP_GSTREAMER("QTBUG-124005: spurious seek failures on CI");

    QAudioInput input;
    for (int i = 0; i < 50; i++) {
        QMediaCaptureSession session;
        QMediaRecorder recorder;
        QCamera camera;
        QAudioOutput output;
        QVideoWidget video;

        if (!input.device().isNull())
            session.setAudioInput(&input);
        if (!output.device().isNull())
            session.setAudioOutput(&output);
        session.setRecorder(&recorder);
        session.setCamera(&camera);
        session.setVideoOutput(&video);

        QRandomGenerator rng;
        QTest::qWait(rng.bounded(200));
    }
}

void tst_QMediaCaptureSession::stress_test_setup_and_teardown_keep_audiooutput()
{
    QSKIP_GSTREAMER("QTBUG-124005: spurious seek failures on CI");

    QAudioOutput output;
    for (int i = 0; i < 50; i++) {
        QMediaCaptureSession session;
        QMediaRecorder recorder;
        QCamera camera;
        QAudioInput input;
        QVideoWidget video;

        if (!input.device().isNull())
            session.setAudioInput(&input);
        if (!output.device().isNull())
            session.setAudioOutput(&output);
        session.setRecorder(&recorder);
        session.setCamera(&camera);
        session.setVideoOutput(&video);

        QRandomGenerator rng;
        QTest::qWait(rng.bounded(200));
    }
}

void tst_QMediaCaptureSession::stress_test_setup_and_teardown_keep_video()
{
    QSKIP_GSTREAMER("QTBUG-124005: spurious seek failures on CI");

    QVideoWidget video;
    for (int i = 0; i < 50; i++) {
        QMediaCaptureSession session;
        QMediaRecorder recorder;
        QCamera camera;
        QAudioInput input;
        QAudioOutput output;

        if (!input.device().isNull())
            session.setAudioInput(&input);
        if (!output.device().isNull())
            session.setAudioOutput(&output);
        session.setRecorder(&recorder);
        session.setCamera(&camera);
        session.setVideoOutput(&video);

        QRandomGenerator rng;
        QTest::qWait(rng.bounded(200));
    }
}

void tst_QMediaCaptureSession::record_video_without_preview()
{
    QCamera camera;

    if (!camera.isAvailable())
        QSKIP("No video input is available");

    QMediaRecorder recorder;
    QMediaCaptureSession session;

    session.setRecorder(&recorder);

    QSignalSpy cameraChanged(&session, &QMediaCaptureSession::cameraChanged);

    session.setCamera(&camera);
    camera.setActive(true);
    QTRY_COMPARE(cameraChanged.size(), 1);
    QTRY_COMPARE(camera.isActive(), true);

    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());

    session.setCamera(nullptr);
    QTRY_COMPARE(cameraChanged.size(), 2);

    // can't record without audio and video
    recordFail(session);
    QVERIFY(!QTest::currentTestFailed());
}

void tst_QMediaCaptureSession::can_add_and_remove_AudioInput_with_and_without_AudioOutput_attached()
{
    QAudioInput input;
    if (input.device().isNull())
        QSKIP("No audio input available");

    QMediaCaptureSession session;
    QSignalSpy audioInputChanged(&session, &QMediaCaptureSession::audioInputChanged);
    QSignalSpy audioOutputChanged(&session, &QMediaCaptureSession::audioOutputChanged);

    session.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged.size(), 1);
    session.setAudioInput(nullptr);
    QTRY_COMPARE(audioInputChanged.size(), 2);

    QAudioOutput output;
    if (output.device().isNull())
        return;

    session.setAudioOutput(&output);
    QTRY_COMPARE(audioOutputChanged.size(), 1);

    session.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged.size(), 3);

    session.setAudioOutput(nullptr);
    QTRY_COMPARE(audioOutputChanged.size(), 2);

    session.setAudioInput(nullptr);
    QTRY_COMPARE(audioInputChanged.size(), 4);
}

void tst_QMediaCaptureSession::can_change_AudioDevices_on_attached_AudioInput()
{
    auto audioInputs = QMediaDevices::audioInputs();
    if (audioInputs.size() < 2)
        QSKIP("Two audio inputs are not available");

    QAudioInput input(audioInputs[0]);
    QSignalSpy deviceChanged(&input, &QAudioInput::deviceChanged);

    QMediaCaptureSession session;
    QSignalSpy audioInputChanged(&session, &QMediaCaptureSession::audioInputChanged);

    session.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged.size(), 1);

    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());

    input.setDevice(audioInputs[1]);
    QTRY_COMPARE(deviceChanged.size(), 1);

    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());

    input.setDevice(audioInputs[0]);
    QTRY_COMPARE(deviceChanged.size(), 2);

    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());
}

void tst_QMediaCaptureSession::can_change_AudioInput_during_recording()
{
    QAudioInput input;
    if (input.device().isNull())
        QSKIP("No audio input available");

    QMediaRecorder recorder;
    QMediaCaptureSession session;

    session.setRecorder(&recorder);

    QSignalSpy audioInputChanged(&session, &QMediaCaptureSession::audioInputChanged);
    QSignalSpy recorderErrorSignal(&recorder, &QMediaRecorder::errorOccurred);
    QSignalSpy durationChanged(&recorder, &QMediaRecorder::durationChanged);

    session.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged.size(), 1);

    recorder.record();
    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::RecordingState);
    QVERIFY(durationChanged.wait(2000));
    session.setAudioInput(nullptr);
    QTRY_COMPARE(audioInputChanged.size(), 2);
    session.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged.size(), 3);
    recorder.stop();

    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::StoppedState);
    QVERIFY(recorderErrorSignal.isEmpty());

    session.setAudioInput(nullptr);
    QTRY_COMPARE(audioInputChanged.size(), 4);

    QString fileName = recorder.actualLocation().toLocalFile();
    QVERIFY(!fileName.isEmpty());
    QTRY_VERIFY(QFileInfo(fileName).size() > 0);
    QFile(fileName).remove();
}

void tst_QMediaCaptureSession::disconnects_deleted_AudioInput()
{
    if (QMediaDevices::audioInputs().isEmpty())
        QSKIP("No audio input available");

    QMediaCaptureSession session;
    QSignalSpy audioInputChanged(&session, &QMediaCaptureSession::audioInputChanged);
    {
        QAudioInput input;
        session.setAudioInput(&input);
        QTRY_COMPARE(audioInputChanged.size(), 1);
    }
    QVERIFY(session.audioInput() == nullptr);
    QTRY_COMPARE(audioInputChanged.size(), 2);
}

void tst_QMediaCaptureSession::can_move_AudioInput_between_sessions()
{
    if (QMediaDevices::audioInputs().isEmpty())
        QSKIP("No audio input available");

    QMediaCaptureSession session0;
    QMediaCaptureSession session1;
    QSignalSpy audioInputChanged0(&session0, &QMediaCaptureSession::audioInputChanged);
    QSignalSpy audioInputChanged1(&session1, &QMediaCaptureSession::audioInputChanged);

    QAudioInput input;
    {
        QMediaCaptureSession session2;
        QSignalSpy audioInputChanged2(&session2, &QMediaCaptureSession::audioInputChanged);
        session2.setAudioInput(&input);
        QTRY_COMPARE(audioInputChanged2.size(), 1);
    }
    session0.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged0.size(), 1);
    QVERIFY(session0.audioInput() != nullptr);

    session1.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged0.size(), 2);
    QVERIFY(session0.audioInput() == nullptr);
    QTRY_COMPARE(audioInputChanged1.size(), 1);
    QVERIFY(session1.audioInput() != nullptr);
}

void tst_QMediaCaptureSession::disconnects_deleted_AudioOutput()
{
    if (QMediaDevices::audioOutputs().isEmpty())
        QSKIP("No audio output available");

    QMediaCaptureSession session;
    QSignalSpy audioOutputChanged(&session, &QMediaCaptureSession::audioOutputChanged);
    {
        QAudioOutput output;
        session.setAudioOutput(&output);
        QTRY_COMPARE(audioOutputChanged.size(), 1);
    }
    QVERIFY(session.audioOutput() == nullptr);
    QTRY_COMPARE(audioOutputChanged.size(), 2);
}

void tst_QMediaCaptureSession::can_move_AudioOutput_between_sessions_and_player()
{
    if (QMediaDevices::audioOutputs().isEmpty())
        QSKIP("No audio output available");

    QAudioOutput output;

    QMediaCaptureSession session0;
    QMediaCaptureSession session1;
    QMediaPlayer player;
    QSignalSpy audioOutputChanged0(&session0, &QMediaCaptureSession::audioOutputChanged);
    QSignalSpy audioOutputChanged1(&session1, &QMediaCaptureSession::audioOutputChanged);
    QSignalSpy audioOutputChangedPlayer(&player, &QMediaPlayer::audioOutputChanged);

    {
        QMediaCaptureSession session2;
        QSignalSpy audioOutputChanged2(&session2, &QMediaCaptureSession::audioOutputChanged);
        session2.setAudioOutput(&output);
        QTRY_COMPARE(audioOutputChanged2.size(), 1);
    }

    session0.setAudioOutput(&output);
    QTRY_COMPARE(audioOutputChanged0.size(), 1);
    QVERIFY(session0.audioOutput() != nullptr);

    session1.setAudioOutput(&output);
    QTRY_COMPARE(audioOutputChanged0.size(), 2);
    QVERIFY(session0.audioOutput() == nullptr);
    QTRY_COMPARE(audioOutputChanged1.size(), 1);
    QVERIFY(session1.audioOutput() != nullptr);

    player.setAudioOutput(&output);
    QTRY_COMPARE(audioOutputChanged0.size(), 2);
    QVERIFY(session0.audioOutput() == nullptr);
    QTRY_COMPARE(audioOutputChanged1.size(), 2);
    QVERIFY(session1.audioOutput() == nullptr);
    QTRY_COMPARE(audioOutputChangedPlayer.size(), 1);
    QVERIFY(player.audioOutput() != nullptr);

    session0.setAudioOutput(&output);
    QTRY_COMPARE(audioOutputChanged0.size(), 3);
    QVERIFY(session0.audioOutput() != nullptr);
    QTRY_COMPARE(audioOutputChangedPlayer.size(), 2);
    QVERIFY(player.audioOutput() == nullptr);
}

void tst_QMediaCaptureSession::disconnects_deleted_AudioBufferInput()
{
    QMediaCaptureSession session;
    QSignalSpy audioBufferInputChanged(&session, &QMediaCaptureSession::audioBufferInputChanged);
    {
        QAudioBufferInput input;
        session.setAudioBufferInput(&input);
        QTRY_COMPARE(audioBufferInputChanged.size(), 1);
    }
    QCOMPARE(session.audioBufferInput(), nullptr);
    QCOMPARE(audioBufferInputChanged.size(), 2);
}

void tst_QMediaCaptureSession::can_move_AudioBufferInput_between_sessions()
{
    QMediaCaptureSession session0;
    QMediaCaptureSession session1;
    QSignalSpy audioBufferInputChanged0(&session0, &QMediaCaptureSession::audioBufferInputChanged);
    QSignalSpy audioBufferInputChanged1(&session1, &QMediaCaptureSession::audioBufferInputChanged);

    QAudioBufferInput input;
    {
        QMediaCaptureSession session2;
        QSignalSpy audioBufferInputChanged2(&session2,
                                            &QMediaCaptureSession::audioBufferInputChanged);
        session2.setAudioBufferInput(&input);
        QCOMPARE(audioBufferInputChanged2.size(), 1);
    }
    session0.setAudioBufferInput(&input);
    QCOMPARE(audioBufferInputChanged0.size(), 1);
    QCOMPARE(session0.audioBufferInput(), &input);
    QCOMPARE(input.captureSession(), &session0);

    session1.setAudioBufferInput(&input);

    QCOMPARE(audioBufferInputChanged0.size(), 2);
    QCOMPARE(session0.audioBufferInput(), nullptr);
    QCOMPARE(audioBufferInputChanged1.size(), 1);
    QCOMPARE(session1.audioBufferInput(), &input);
    QCOMPARE(input.captureSession(), &session1);
}

void tst_QMediaCaptureSession::disconnects_deleted_VideoFrameInput()
{
    QMediaCaptureSession session;
    QSignalSpy videoFrameInputChanged(&session, &QMediaCaptureSession::videoFrameInputChanged);
    {
        QVideoFrameInput input;
        session.setVideoFrameInput(&input);
        QTRY_COMPARE(videoFrameInputChanged.size(), 1);
    }
    QCOMPARE(session.videoFrameInput(), nullptr);
    QCOMPARE(videoFrameInputChanged.size(), 2);
}

void tst_QMediaCaptureSession::can_move_VideoFrameInput_between_sessions()
{
    QMediaCaptureSession session0;
    QMediaCaptureSession session1;
    QSignalSpy videoFrameInputChanged0(&session0, &QMediaCaptureSession::videoFrameInputChanged);
    QSignalSpy videoFrameInputChanged1(&session1, &QMediaCaptureSession::videoFrameInputChanged);

    QVideoFrameInput input;
    {
        QMediaCaptureSession session2;
        QSignalSpy videoFrameInputChanged2(&session2,
                                           &QMediaCaptureSession::videoFrameInputChanged);
        session2.setVideoFrameInput(&input);
        QCOMPARE(videoFrameInputChanged2.size(), 1);
    }
    session0.setVideoFrameInput(&input);
    QCOMPARE(videoFrameInputChanged0.size(), 1);
    QCOMPARE(session0.videoFrameInput(), &input);
    QCOMPARE(input.captureSession(), &session0);

    session1.setVideoFrameInput(&input);

    QCOMPARE(videoFrameInputChanged0.size(), 2);
    QCOMPARE(session0.videoFrameInput(), nullptr);
    QCOMPARE(videoFrameInputChanged1.size(), 1);
    QCOMPARE(session1.videoFrameInput(), &input);
    QCOMPARE(input.captureSession(), &session1);
}

void tst_QMediaCaptureSession::can_add_and_remove_Camera()
{
    QCamera camera;

    if (!camera.isAvailable())
        QSKIP("No video input is available");

    QMediaRecorder recorder;
    QMediaCaptureSession session;

    session.setRecorder(&recorder);

    QSignalSpy cameraChanged(&session, &QMediaCaptureSession::cameraChanged);

    session.setCamera(&camera);
    camera.setActive(true);
    QTRY_COMPARE(cameraChanged.size(), 1);
    QTRY_COMPARE(camera.isActive(), true);

    session.setCamera(nullptr);
    QTRY_COMPARE(cameraChanged.size(), 2);

    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.size(), 3);

    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());
}

void tst_QMediaCaptureSession::can_move_Camera_between_sessions()
{
    QMediaCaptureSession session0;
    QMediaCaptureSession session1;
    QSignalSpy cameraChanged0(&session0, &QMediaCaptureSession::cameraChanged);
    QSignalSpy cameraChanged1(&session1, &QMediaCaptureSession::cameraChanged);
    {
        QCamera camera;
        {
            QMediaCaptureSession session2;
            QSignalSpy cameraChanged2(&session2, &QMediaCaptureSession::cameraChanged);
            session2.setCamera(&camera);
            QTRY_COMPARE(cameraChanged2.size(), 1);
        }
        QVERIFY(camera.captureSession() == nullptr);

        session0.setCamera(&camera);
        QTRY_COMPARE(cameraChanged0.size(), 1);
        QVERIFY(session0.camera() == &camera);
        QVERIFY(camera.captureSession() == &session0);

        session1.setCamera(&camera);
        QTRY_COMPARE(cameraChanged0.size(), 2);
        QVERIFY(session0.camera() == nullptr);
        QTRY_COMPARE(cameraChanged1.size(), 1);
        QVERIFY(session1.camera() == &camera);
        QVERIFY(camera.captureSession() == &session1);
    }
    QTRY_COMPARE(cameraChanged1.size(), 2);
    QVERIFY(session1.camera() == nullptr);
}

void tst_QMediaCaptureSession::can_disconnect_Camera_when_recording()
{
    QCamera camera;

    if (!camera.isAvailable())
        QSKIP("No video input is available");

    QMediaRecorder recorder;
    QMediaCaptureSession session;

    session.setRecorder(&recorder);

    QSignalSpy cameraChanged(&session, &QMediaCaptureSession::cameraChanged);
    QSignalSpy recorderErrorSignal(&recorder, &QMediaRecorder::errorOccurred);
    QSignalSpy durationChanged(&recorder, &QMediaRecorder::durationChanged);

    camera.setActive(true);
    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.size(), 1);
    QTRY_COMPARE(camera.isActive(), true);

    durationChanged.clear();
    recorder.record();
    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::RecordingState);
    QTRY_VERIFY(durationChanged.size() > 0);

    session.setCamera(nullptr);
    QTRY_COMPARE(cameraChanged.size(), 2);

    recorder.stop();
    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::StoppedState);
    QVERIFY(recorderErrorSignal.isEmpty());

    QString fileName = recorder.actualLocation().toLocalFile();
    QVERIFY(!fileName.isEmpty());
    QTRY_VERIFY(QFileInfo(fileName).size() > 0);
    QFile(fileName).remove();
}

void tst_QMediaCaptureSession::can_add_and_remove_different_Cameras()
{
    auto cameraDevices = QMediaDevices().videoInputs();

    if (cameraDevices.size() < 2)
        QSKIP("Two video input are not available");

    QCamera camera(cameraDevices[0]);
    QCamera camera2(cameraDevices[1]);

    QMediaRecorder recorder;
    QMediaCaptureSession session;

    session.setRecorder(&recorder);

    QSignalSpy cameraChanged(&session, &QMediaCaptureSession::cameraChanged);

    camera.setActive(true);
    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.size(), 1);
    QTRY_COMPARE(camera.isActive(), true);

    session.setCamera(nullptr);
    QTRY_COMPARE(cameraChanged.size(), 2);

    session.setCamera(&camera2);
    camera2.setActive(true);
    QTRY_COMPARE(cameraChanged.size(), 3);
    QTRY_COMPARE(camera2.isActive(), true);

    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());
}

void tst_QMediaCaptureSession::can_change_CameraDevice_on_attached_Camera()
{
    auto cameraDevices = QMediaDevices().videoInputs();

    if (cameraDevices.size() < 2)
        QSKIP("Two video input are not available");

    QCamera camera(cameraDevices[0]);

    QMediaRecorder recorder;
    QMediaCaptureSession session;

    session.setRecorder(&recorder);

    QSignalSpy cameraDeviceChanged(&camera, &QCamera::cameraDeviceChanged);
    QSignalSpy cameraChanged(&session, &QMediaCaptureSession::cameraChanged);

    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.size(), 1);

    recordFail(session);
    QVERIFY(!QTest::currentTestFailed());

    camera.setActive(true);
    QTRY_COMPARE(camera.isActive(), true);

    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());

    camera.setCameraDevice(cameraDevices[1]);
    camera.setActive(true);
    QTRY_COMPARE(cameraDeviceChanged.size(), 1);
    QTRY_COMPARE(camera.isActive(), true);

    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());
}

void tst_QMediaCaptureSession::can_change_VideoOutput_with_and_without_camera()
{
    QCamera camera;
    if (!camera.isAvailable())
        QSKIP("No video input is available");

    QVideoWidget videoOutput;
    QVideoWidget videoOutput2;
    videoOutput.show();
    videoOutput2.show();

    QMediaCaptureSession session;

    QSignalSpy videoOutputChanged(&session, &QMediaCaptureSession::videoOutputChanged);
    QSignalSpy cameraChanged(&session, &QMediaCaptureSession::cameraChanged);

    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.size(), 1);

    session.setVideoOutput(&videoOutput);
    QTRY_COMPARE(videoOutputChanged.size(), 1);

    session.setVideoOutput(nullptr);
    QTRY_COMPARE(videoOutputChanged.size(), 2);

    session.setVideoOutput(&videoOutput2);
    QTRY_COMPARE(videoOutputChanged.size(), 3);

    session.setCamera(nullptr);
    QTRY_COMPARE(cameraChanged.size(), 2);

    session.setVideoOutput(nullptr);
    QTRY_COMPARE(videoOutputChanged.size(), 4);
}

void tst_QMediaCaptureSession::can_change_VideoOutput_when_recording()
{
    QCamera camera;
    if (!camera.isAvailable())
        QSKIP("No video input is available");

    QVideoWidget videoOutput;
    videoOutput.show();

    QMediaCaptureSession session;
    QMediaRecorder recorder;

    session.setRecorder(&recorder);

    QSignalSpy cameraChanged(&session, &QMediaCaptureSession::cameraChanged);
    QSignalSpy recorderErrorSignal(&recorder, &QMediaRecorder::errorOccurred);
    QSignalSpy durationChanged(&recorder, &QMediaRecorder::durationChanged);
    QSignalSpy videoOutputChanged(&session, &QMediaCaptureSession::videoOutputChanged);

    camera.setActive(true);
    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.size(), 1);
    QTRY_COMPARE(camera.isActive(), true);

    recorder.record();
    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::RecordingState);
    QVERIFY(durationChanged.wait(2000));

    session.setVideoOutput(&videoOutput);
    QTRY_COMPARE(videoOutputChanged.size(), 1);

    session.setVideoOutput(nullptr);
    QTRY_COMPARE(videoOutputChanged.size(), 2);

    session.setVideoOutput(&videoOutput);
    QTRY_COMPARE(videoOutputChanged.size(), 3);

    recorder.stop();
    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::StoppedState);
    QVERIFY(recorderErrorSignal.isEmpty());

    QString fileName = recorder.actualLocation().toLocalFile();
    QVERIFY(!fileName.isEmpty());
    QTRY_VERIFY(QFileInfo(fileName).size() > 0);
    QFile(fileName).remove();
}

void tst_QMediaCaptureSession::can_add_and_remove_recorders()
{
    QAudioInput input;
    if (input.device().isNull())
        QSKIP("Recording source not available");

    QMediaRecorder recorder;
    QMediaRecorder recorder2;
    QMediaCaptureSession session;

    QSignalSpy audioInputChanged(&session, &QMediaCaptureSession::audioInputChanged);
    QSignalSpy recorderChanged(&session, &QMediaCaptureSession::recorderChanged);

    session.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged.size(), 1);

    session.setRecorder(&recorder);
    QTRY_COMPARE(recorderChanged.size(), 1);

    session.setRecorder(&recorder2);
    QTRY_COMPARE(recorderChanged.size(), 2);

    session.setRecorder(&recorder);
    QTRY_COMPARE(recorderChanged.size(), 3);

    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());
}

void tst_QMediaCaptureSession::can_move_Recorder_between_sessions()
{
    QMediaCaptureSession session0;
    QMediaCaptureSession session1;
    QSignalSpy recorderChanged0(&session0, &QMediaCaptureSession::recorderChanged);
    QSignalSpy recorderChanged1(&session1, &QMediaCaptureSession::recorderChanged);
    {
        QMediaRecorder recorder;
        {
            QMediaCaptureSession session2;
            QSignalSpy recorderChanged2(&session2, &QMediaCaptureSession::recorderChanged);
            session2.setRecorder(&recorder);
            QTRY_COMPARE(recorderChanged2.size(), 1);
        }
        QVERIFY(recorder.captureSession() == nullptr);

        session0.setRecorder(&recorder);
        QTRY_COMPARE(recorderChanged0.size(), 1);
        QVERIFY(session0.recorder() == &recorder);
        QVERIFY(recorder.captureSession() == &session0);

        session1.setRecorder(&recorder);
        QTRY_COMPARE(recorderChanged0.size(), 2);
        QVERIFY(session0.recorder() == nullptr);
        QTRY_COMPARE(recorderChanged1.size(), 1);
        QVERIFY(session1.recorder() == &recorder);
        QVERIFY(recorder.captureSession() == &session1);
    }
    QTRY_COMPARE(recorderChanged1.size(), 2);
    QVERIFY(session1.recorder() == nullptr);
}

void tst_QMediaCaptureSession::cannot_record_without_Camera_and_AudioInput()
{
    QMediaCaptureSession session;
    recordFail(session);
}

void tst_QMediaCaptureSession::can_record_AudioInput_with_null_AudioDevice()
{
    if (QMediaDevices().audioInputs().size() == 0)
        QSKIP("No audio input is not available");

    QAudioDevice nullDevice;
    QAudioInput input(nullDevice);

    QMediaCaptureSession session;
    QSignalSpy audioInputChanged(&session, &QMediaCaptureSession::audioInputChanged);

    session.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged.size(), 1);

    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());
}

void tst_QMediaCaptureSession::can_record_Camera_with_null_CameraDevice()
{
    if (QMediaDevices().videoInputs().size() == 0)
        QSKIP("No video input is not available");

    QCameraDevice nullDevice;
    QCamera camera(nullDevice);

    QMediaCaptureSession session;
    QSignalSpy cameraChanged(&session, &QMediaCaptureSession::cameraChanged);

    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.size(), 1);

    camera.setActive(true);
    QTRY_COMPARE(camera.isActive(), true);

    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());
}

void tst_QMediaCaptureSession::recording_stops_when_recorder_removed()
{
    QAudioInput input;
    if (input.device().isNull())
        QSKIP("Recording source not available");

    QSKIP_GSTREAMER("spurious failures on gstreamer");

    QMediaRecorder recorder;
    QMediaCaptureSession session;

    QSignalSpy audioInputChanged(&session, &QMediaCaptureSession::audioInputChanged);
    QSignalSpy recorderChanged(&session, &QMediaCaptureSession::recorderChanged);
    QSignalSpy recorderErrorSignal(&recorder, &QMediaRecorder::errorOccurred);
    QSignalSpy durationChanged(&recorder, &QMediaRecorder::durationChanged);

    session.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged.size(), 1);

    session.setRecorder(&recorder);
    QTRY_COMPARE(recorderChanged.size(), 1);

    recorder.record();
    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::RecordingState);
    QVERIFY(durationChanged.wait(2000));

    session.setRecorder(nullptr);
    QTRY_COMPARE(recorderChanged.size(), 2);

    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::StoppedState);
    QVERIFY(recorderErrorSignal.isEmpty());

    QString fileName = recorder.actualLocation().toLocalFile();
    QVERIFY(!fileName.isEmpty());
    QTRY_VERIFY(QFileInfo(fileName).size() > 0);
    QFile(fileName).remove();
}

void tst_QMediaCaptureSession::can_add_and_remove_ImageCapture()
{
    QCamera camera;

    if (!camera.isAvailable())
        QSKIP("No video input available");

    QImageCapture capture;
    QMediaCaptureSession session;

    QSignalSpy cameraChanged(&session, &QMediaCaptureSession::cameraChanged);
    QSignalSpy imageCaptureChanged(&session, &QMediaCaptureSession::imageCaptureChanged);
    QSignalSpy readyForCaptureChanged(&capture, &QImageCapture::readyForCaptureChanged);

    QVERIFY(!capture.isAvailable());
    QVERIFY(!capture.isReadyForCapture());

    session.setImageCapture(&capture);
    QTRY_COMPARE(imageCaptureChanged.size(), 1);
    QVERIFY(!capture.isAvailable());
    QVERIFY(!capture.isReadyForCapture());

    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.size(), 1);
    QVERIFY(capture.isAvailable());

    QVERIFY(!capture.isReadyForCapture());

    camera.setActive(true);
    QTRY_COMPARE(camera.isActive(), true);

    QTRY_COMPARE(readyForCaptureChanged.size(), 1);
    QVERIFY(capture.isReadyForCapture());

    session.setImageCapture(nullptr);
    QTRY_COMPARE(imageCaptureChanged.size(), 2);
    QTRY_COMPARE(readyForCaptureChanged.size(), 2);

    QVERIFY(!capture.isAvailable());
    QVERIFY(!capture.isReadyForCapture());

    session.setImageCapture(&capture);
    QTRY_COMPARE(imageCaptureChanged.size(), 3);
    QTRY_COMPARE(readyForCaptureChanged.size(), 3);
    QVERIFY(capture.isAvailable());
    QVERIFY(capture.isReadyForCapture());
}

void tst_QMediaCaptureSession::can_move_ImageCapture_between_sessions()
{
    QSKIP_GSTREAMER("QTBUG-124005: Spurious failure on CI");

    QMediaCaptureSession session0;
    QMediaCaptureSession session1;
    QSignalSpy imageCaptureChanged0(&session0, &QMediaCaptureSession::imageCaptureChanged);
    QSignalSpy imageCaptureChanged1(&session1, &QMediaCaptureSession::imageCaptureChanged);
    {
        QImageCapture imageCapture;
        {
            QMediaCaptureSession session2;
            QSignalSpy imageCaptureChanged2(&session2, &QMediaCaptureSession::imageCaptureChanged);
            session2.setImageCapture(&imageCapture);
            QTRY_COMPARE(imageCaptureChanged2.size(), 1);
        }
        QVERIFY(imageCapture.captureSession() == nullptr);

        session0.setImageCapture(&imageCapture);
        QTRY_COMPARE(imageCaptureChanged0.size(), 1);
        QVERIFY(session0.imageCapture() == &imageCapture);
        QVERIFY(imageCapture.captureSession() == &session0);

        session1.setImageCapture(&imageCapture);
        QTRY_COMPARE(imageCaptureChanged0.size(), 2);
        QVERIFY(session0.imageCapture() == nullptr);
        QTRY_COMPARE(imageCaptureChanged1.size(), 1);
        QVERIFY(session1.imageCapture() == &imageCapture);
        QVERIFY(imageCapture.captureSession() == &session1);
    }
    QTRY_COMPARE(imageCaptureChanged1.size(), 2);
    QVERIFY(session1.imageCapture() == nullptr);
}

void tst_QMediaCaptureSession::capture_is_not_available_when_Camera_is_null()
{
    QCamera camera;

    if (!camera.isAvailable())
        QSKIP("No video input available");

    QSKIP_GSTREAMER("Spurious failures");

    QImageCapture capture;
    QMediaCaptureSession session;

    QSignalSpy cameraChanged(&session, &QMediaCaptureSession::cameraChanged);
    QSignalSpy capturedSignal(&capture, &QImageCapture::imageCaptured);
    QSignalSpy readyForCaptureChanged(&capture, &QImageCapture::readyForCaptureChanged);

    session.setImageCapture(&capture);
    session.setCamera(&camera);
    camera.setActive(true);
    QTRY_COMPARE(camera.isActive(), true);

    QTRY_COMPARE(readyForCaptureChanged.size(), 1);
    QVERIFY(capture.isReadyForCapture());

    QVERIFY(capture.capture() >= 0);
    QTRY_COMPARE(capturedSignal.size(), 1);

    QVERIFY(capture.isReadyForCapture());
    int readyCount = readyForCaptureChanged.size();

    session.setCamera(nullptr);

    QTRY_COMPARE(readyForCaptureChanged.size(), readyCount + 1);
    QVERIFY(!capture.isReadyForCapture());
    QVERIFY(!capture.isAvailable());
    QVERIFY(capture.capture() < 0);
}

void tst_QMediaCaptureSession::can_add_ImageCapture_and_capture_during_recording()
{
    QCamera camera;

    if (!camera.isAvailable())
        QSKIP("No video input available");

    QSKIP_GSTREAMER("spurious failures with gstreamer");

    QImageCapture capture;
    QMediaCaptureSession session;
    QMediaRecorder recorder;

    QSignalSpy recorderChanged(&session, &QMediaCaptureSession::recorderChanged);
    QSignalSpy recorderErrorSignal(&recorder, &QMediaRecorder::errorOccurred);
    QSignalSpy durationChanged(&recorder, &QMediaRecorder::durationChanged);
    QSignalSpy imageCaptureChanged(&session, &QMediaCaptureSession::imageCaptureChanged);
    QSignalSpy readyForCaptureChanged(&capture, &QImageCapture::readyForCaptureChanged);
    QSignalSpy capturedSignal(&capture, &QImageCapture::imageCaptured);

    session.setCamera(&camera);
    camera.setActive(true);
    QTRY_COMPARE(camera.isActive(), true);

    session.setRecorder(&recorder);
    QTRY_COMPARE(recorderChanged.size(), 1);

    recorder.record();
    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::RecordingState);
    QVERIFY(durationChanged.wait(2000));

    session.setImageCapture(&capture);
    QTRY_COMPARE(imageCaptureChanged.size(), 1);
    QTRY_COMPARE(readyForCaptureChanged.size(), 1);
    QVERIFY(capture.isReadyForCapture());

    QVERIFY(capture.capture() >= 0);
    QTRY_COMPARE(capturedSignal.size(), 1);

    session.setImageCapture(nullptr);
    QVERIFY(readyForCaptureChanged.size() >= 2);
    QVERIFY(!capture.isReadyForCapture());

    recorder.stop();
    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::StoppedState);
    QVERIFY(recorderErrorSignal.isEmpty());

    QString fileName = recorder.actualLocation().toLocalFile();
    QVERIFY(!fileName.isEmpty());
    QTRY_VERIFY(QFileInfo(fileName).size() > 0);
    QFile(fileName).remove();
}

void tst_QMediaCaptureSession::testAudioMute()
{
    QAudioInput audioInput;
    if (audioInput.device().isNull())
        QSKIP("No audio input available");

    QMediaRecorder recorder;
    QMediaCaptureSession session;

    session.setRecorder(&recorder);

    session.setAudioInput(&audioInput);

    session.setCamera(nullptr);
    recorder.setOutputLocation(QStringLiteral("test"));

    QSignalSpy spy(&audioInput, &QAudioInput::mutedChanged);
    QSignalSpy durationChanged(&recorder, &QMediaRecorder::durationChanged);

    QMediaFormat format;
    format.setAudioCodec(QMediaFormat::AudioCodec::Wave);
    recorder.setMediaFormat(format);

    audioInput.setMuted(true);

    recorder.record();

    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.last()[0], true);

    QTRY_VERIFY_WITH_TIMEOUT(recorder.recorderState() == QMediaRecorder::RecordingState, 2000);
    QVERIFY(durationChanged.wait(2000));

    recorder.stop();

    QTRY_COMPARE(recorder.recorderState(), QMediaRecorder::StoppedState);

    QString actualLocation = recorder.actualLocation().toLocalFile();

    QVERIFY2(!actualLocation.isEmpty(), "Recorder did not save a file");
    QTRY_VERIFY2(QFileInfo(actualLocation).size() > 0, "Recorded file is empty (zero bytes)");

    QAudioDecoder decoder;
    QAudioBuffer buffer;
    decoder.setSource(QUrl::fromLocalFile(actualLocation));

    decoder.start();

    // Wait a while
    QTRY_VERIFY(decoder.bufferAvailable());

    while (decoder.bufferAvailable()) {
        buffer = decoder.read();
        QVERIFY(buffer.isValid());

        const void *data = buffer.constData<void *>();
        QVERIFY(data != nullptr);

        const unsigned int *idata = reinterpret_cast<const unsigned int *>(data);
        QCOMPARE(*idata, 0U);
    }

    decoder.stop();

    QFile(actualLocation).remove();

    audioInput.setMuted(false);

    QCOMPARE(spy.size(), 2);
    QCOMPARE(spy.last()[0], false);
}

void tst_QMediaCaptureSession::can_switch_audio_input()
{
    // prepare
    QMediaCaptureSession session;
    QAudioInput in1;
    session.setAudioInput(&in1);
    QVERIFY(session.audioInput() != nullptr);
    QAudioOutput out;
    session.setAudioOutput(&out);
    QVERIFY(session.audioOutput() != nullptr);

    // exercise
    QAudioInput in2;
    QSignalSpy changeSpy(&session, &QMediaCaptureSession::audioInputChanged);
    session.setAudioInput(&in2);

    // validate
    QVERIFY(session.audioInput() != nullptr);
    QCOMPARE(changeSpy.count(), 1);
}

void tst_QMediaCaptureSession::can_switch_audio_output()
{
    // prepare
    QMediaCaptureSession session;
    QAudioInput in;
    session.setAudioInput(&in);
    QVERIFY(session.audioInput() != nullptr);
    QAudioOutput out1;
    session.setAudioOutput(&out1);
    QVERIFY(session.audioOutput() != nullptr);

    // exercise
    QSignalSpy changeSpy(&session, &QMediaCaptureSession::audioOutputChanged);
    QAudioOutput out2;
    session.setAudioOutput(&out2);

    // validate
    QVERIFY(session.audioOutput() != nullptr);
    QCOMPARE(changeSpy.count(), 1);
}

void tst_QMediaCaptureSession::can_clear_audio_input()
{
    // prepare
    QMediaCaptureSession session;
    QAudioInput in1;
    session.setAudioInput(&in1);
    QVERIFY(session.audioInput() != nullptr);
    QAudioOutput out;
    session.setAudioOutput(&out);
    QVERIFY(session.audioOutput() != nullptr);

    // exercise
    QSignalSpy changeSpy(&session, &QMediaCaptureSession::audioInputChanged);
    session.setAudioInput(nullptr);

    // validate
    QVERIFY(session.audioInput() == nullptr);
    QCOMPARE(changeSpy.count(), 1);
}

void tst_QMediaCaptureSession::can_clear_audio_output()
{
    // prepare
    QMediaCaptureSession session;
    QAudioInput in;
    session.setAudioInput(&in);
    QVERIFY(session.audioInput() != nullptr);
    QAudioOutput out1;
    session.setAudioOutput(&out1);
    QVERIFY(session.audioOutput() != nullptr);

    // exercise
    QSignalSpy changeSpy(&session, &QMediaCaptureSession::audioOutputChanged);
    session.setAudioOutput(nullptr);

    // validate
    QVERIFY(session.audioOutput() == nullptr);
    QCOMPARE(changeSpy.count(), 1);
}

QTEST_MAIN(tst_QMediaCaptureSession)

#include "tst_qmediacapturesession.moc"
