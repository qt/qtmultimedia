/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <QtGui/QImageReader>
#include <QtCore/qurl.h>
#include <QDebug>
#include <QVideoSink>
#include <QVideoWidget>

#include <qcamera.h>
#include <qcameradevice.h>
#include <qimagecapture.h>
#include <qmediacapturesession.h>
#include <qmediadevices.h>
#include <qmediarecorder.h>
#include <qaudiooutput.h>
#include <qaudioinput.h>
#include <qaudiodevice.h>
#include <qcamera.h>

#include <QtMultimediaWidgets/QVideoWidget>

QT_USE_NAMESPACE

/*
 This is the backend conformance test.

 Since it relies on platform media framework and sound hardware
 it may be less stable.
*/

class tst_QMediaCaptureSession: public QObject
{
    Q_OBJECT

private slots:
    void can_add_and_remove_AudioInput_with_and_without_AudioOutput_attached();
    void can_change_AudioDevices_on_attached_AudioInput();
    void can_change_AudioInput_during_recording();
    void disconnects_deleted_AudioInput();
    void can_move_AudioInput_between_sessions();

    void can_add_and_remove_Camera();
    void can_disconnect_Camera_when_recording();
    void can_add_and_remove_different_Cameras();
    void can_change_CameraDevice_on_attached_Camera();

    void can_change_VideoOutput_with_and_without_camera();
    void can_change_VideoOutput_when_recording();

    void can_add_and_remove_recorders();
    void cannot_record_without_Camera_and_AudioInput();
    void can_record_AudioInput_with_null_AudioDevice();
    void can_record_Camera_with_null_CameraDevice();
    void recording_stops_when_recorder_removed();

    void can_add_and_remove_ImageCapture();
    void capture_is_not_available_when_Camera_is_null();
    void can_add_ImageCapture_and_capture_during_recording();

private:
    void recordOk(QMediaCaptureSession &session);
    void recordFail(QMediaCaptureSession &session);
};


void tst_QMediaCaptureSession::recordOk(QMediaCaptureSession &session)
{
    QMediaRecorder recorder;
    session.setRecorder(&recorder);

    QSignalSpy recorderErrorSignal(&recorder, SIGNAL(errorOccurred(Error, const QString &)));
    QSignalSpy durationChanged(&recorder, SIGNAL(durationChanged(qint64)));

    recorder.record();
    QTRY_VERIFY_WITH_TIMEOUT(recorder.recorderState() == QMediaRecorder::RecordingState, 1000);
    QVERIFY(durationChanged.wait(1000));
    recorder.stop();

    QTRY_VERIFY_WITH_TIMEOUT(recorder.recorderState() == QMediaRecorder::StoppedState, 1000);
    QVERIFY(recorderErrorSignal.isEmpty());

    QString fileName = recorder.actualLocation().toLocalFile();
    QVERIFY(!fileName.isEmpty());
    QTRY_VERIFY(QFileInfo(fileName).size() > 0);
    QFile(fileName).remove();
}

void tst_QMediaCaptureSession::recordFail(QMediaCaptureSession &session)
{
    QMediaRecorder recorder;
    QSignalSpy recorderErrorSignal(&recorder, SIGNAL(errorOccurred(Error, const QString &)));

    session.setRecorder(&recorder);
    recorder.record();

    QTRY_VERIFY_WITH_TIMEOUT(recorderErrorSignal.count() == 1, 1000);
    QTRY_VERIFY_WITH_TIMEOUT(recorder.recorderState() == QMediaRecorder::StoppedState, 1000);
}

void tst_QMediaCaptureSession::can_add_and_remove_AudioInput_with_and_without_AudioOutput_attached()
{
    QAudioInput input;
    if (input.device().isNull())
        QSKIP("No audio input available");

    QMediaCaptureSession session;
    QSignalSpy audioInputChanged(&session, SIGNAL(audioInputChanged()));
    QSignalSpy audioOutputChanged(&session, SIGNAL(audioOutputChanged()));

    session.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged.count(), 1);
    session.setAudioInput(nullptr);
    QTRY_COMPARE(audioInputChanged.count(), 2);

    QAudioOutput output;
    if (output.device().isNull())
        return;

    session.setAudioOutput(&output);
    QTRY_COMPARE(audioOutputChanged.count(), 1);

    session.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged.count(), 3);

    session.setAudioOutput(nullptr);
    QTRY_COMPARE(audioOutputChanged.count(), 2);

    session.setAudioInput(nullptr);
    QTRY_COMPARE(audioInputChanged.count(), 4);
}

void tst_QMediaCaptureSession::can_change_AudioDevices_on_attached_AudioInput()
{
    auto audioInputs = QMediaDevices::audioInputs();
    if (audioInputs.size() < 2)
        QSKIP("Two audio inputs are not available");

    QAudioInput input(audioInputs[0]);
    QSignalSpy deviceChanged(&input, SIGNAL(deviceChanged()));

    QMediaCaptureSession session;
    QSignalSpy audioInputChanged(&session, SIGNAL(audioInputChanged()));

    session.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged.count(), 1);

    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());

    input.setDevice(audioInputs[1]);
    QTRY_COMPARE(deviceChanged.count(), 1);

    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());

    input.setDevice(audioInputs[0]);
    QTRY_COMPARE(deviceChanged.count(), 2);

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

    QSignalSpy audioInputChanged(&session, SIGNAL(audioInputChanged()));
    QSignalSpy recorderErrorSignal(&recorder, SIGNAL(errorOccurred(Error, const QString &)));
    QSignalSpy durationChanged(&recorder, SIGNAL(durationChanged(qint64)));

    session.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged.count(), 1);

    recorder.record();
    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::RecordingState);
    QVERIFY(durationChanged.wait(1000));
    session.setAudioInput(nullptr);
    QTRY_COMPARE(audioInputChanged.count(), 2);
    session.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged.count(), 3);
    recorder.stop();

    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::StoppedState);
    QVERIFY(recorderErrorSignal.isEmpty());

    session.setAudioInput(nullptr);
    QTRY_COMPARE(audioInputChanged.count(), 4);

    QString fileName = recorder.actualLocation().toLocalFile();
    QVERIFY(!fileName.isEmpty());
    QTRY_VERIFY(QFileInfo(fileName).size() > 0);
    QFile(fileName).remove();
}

void tst_QMediaCaptureSession::disconnects_deleted_AudioInput()
{
    QMediaCaptureSession session;
    QSignalSpy audioInputChanged(&session, SIGNAL(audioInputChanged()));
    {
        QAudioInput input;
        session.setAudioInput(&input);
        QTRY_COMPARE(audioInputChanged.count(), 1);
    }
    QVERIFY(session.audioInput() == nullptr);
    QTRY_COMPARE(audioInputChanged.count(), 2);
}

void tst_QMediaCaptureSession::can_move_AudioInput_between_sessions()
{
    QMediaCaptureSession session0;
    QMediaCaptureSession session1;
    QSignalSpy audioInputChanged0(&session0, SIGNAL(audioInputChanged()));
    QSignalSpy audioInputChanged1(&session1, SIGNAL(audioInputChanged()));

    QAudioInput input;
    {
        QMediaCaptureSession session2;
        QSignalSpy audioInputChanged2(&session2, SIGNAL(audioInputChanged()));
        session2.setAudioInput(&input);
        QTRY_COMPARE(audioInputChanged2.count(), 1);
    }
    session0.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged0.count(), 1);
    QVERIFY(session0.audioInput() != nullptr);

    session1.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged0.count(), 2);
    QVERIFY(session0.audioInput() == nullptr);
    QTRY_COMPARE(audioInputChanged1.count(), 1);
    QVERIFY(session1.audioInput() != nullptr);
}

void tst_QMediaCaptureSession::can_add_and_remove_Camera()
{
    QCamera camera;

    if (!camera.isAvailable())
        QSKIP("No video input is available");

    QMediaRecorder recorder;
    QMediaCaptureSession session;

    session.setRecorder(&recorder);

    QSignalSpy cameraChanged(&session, SIGNAL(cameraChanged()));

    camera.setActive(true);
    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.count(), 1);

    session.setCamera(nullptr);
    QTRY_COMPARE(cameraChanged.count(), 2);

    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.count(), 3);

    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());
}

void tst_QMediaCaptureSession::can_disconnect_Camera_when_recording()
{
    QCamera camera;

    if (!camera.isAvailable())
        QSKIP("No video input is available");

    QMediaRecorder recorder;
    QMediaCaptureSession session;

    session.setRecorder(&recorder);

    QSignalSpy cameraChanged(&session, SIGNAL(cameraChanged()));
    QSignalSpy recorderErrorSignal(&recorder, SIGNAL(errorOccurred(Error, const QString &)));
    QSignalSpy durationChanged(&recorder, SIGNAL(durationChanged(qint64)));

    camera.setActive(true);
    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.count(), 1);

    recorder.record();
    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::RecordingState);
    QVERIFY(durationChanged.wait(1000));

    session.setCamera(nullptr);
    QTRY_COMPARE(cameraChanged.count(), 2);

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

    QSignalSpy cameraChanged(&session, SIGNAL(cameraChanged()));

    camera.setActive(true);
    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.count(), 1);

    session.setCamera(nullptr);
    QTRY_COMPARE(cameraChanged.count(), 2);

    session.setCamera(&camera2);
    camera2.setActive(true);
    QTRY_COMPARE(cameraChanged.count(), 3);

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

    QSignalSpy cameraDeviceChanged(&camera, SIGNAL(cameraDeviceChanged()));
    QSignalSpy cameraChanged(&session, SIGNAL(cameraChanged()));

    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.count(), 1);

    recordFail(session);
    QVERIFY(!QTest::currentTestFailed());

    camera.setActive(true);
    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());

    camera.setCameraDevice(cameraDevices[1]);
    QTRY_COMPARE(cameraDeviceChanged.count(), 1);

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

    QSignalSpy videoOutputChanged(&session, SIGNAL(videoOutputChanged()));
    QSignalSpy cameraChanged(&session, SIGNAL(cameraChanged()));

    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.count(), 1);

    session.setVideoOutput(&videoOutput);
    QTRY_COMPARE(videoOutputChanged.count(), 1);

    session.setVideoOutput(nullptr);
    QTRY_COMPARE(videoOutputChanged.count(), 2);

    session.setVideoOutput(&videoOutput2);
    QTRY_COMPARE(videoOutputChanged.count(), 3);

    session.setCamera(nullptr);
    QTRY_COMPARE(cameraChanged.count(), 2);

    session.setVideoOutput(nullptr);
    QTRY_COMPARE(videoOutputChanged.count(), 4);
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

    QSignalSpy cameraChanged(&session, SIGNAL(cameraChanged()));
    QSignalSpy recorderErrorSignal(&recorder, SIGNAL(errorOccurred(Error, const QString &)));
    QSignalSpy durationChanged(&recorder, SIGNAL(durationChanged(qint64)));
    QSignalSpy videoOutputChanged(&session, SIGNAL(videoOutputChanged()));

    camera.setActive(true);
    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.count(), 1);

    recorder.record();
    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::RecordingState);
    QVERIFY(durationChanged.wait(1000));

    session.setVideoOutput(&videoOutput);
    QTRY_COMPARE(videoOutputChanged.count(), 1);

    session.setVideoOutput(nullptr);
    QTRY_COMPARE(videoOutputChanged.count(), 2);

    session.setVideoOutput(&videoOutput);
    QTRY_COMPARE(videoOutputChanged.count(), 3);

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

    QSignalSpy audioInputChanged(&session, SIGNAL(audioInputChanged()));
    QSignalSpy recorderChanged(&session, SIGNAL(recorderChanged()));

    session.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged.count(), 1);

    session.setRecorder(&recorder);
    QTRY_COMPARE(recorderChanged.count(), 1);

    session.setRecorder(&recorder2);
    QTRY_COMPARE(recorderChanged.count(), 2);

    session.setRecorder(&recorder);
    QTRY_COMPARE(recorderChanged.count(), 3);

    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());
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
    QSignalSpy audioInputChanged(&session, SIGNAL(audioInputChanged()));

    session.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged.count(), 1);

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
    QSignalSpy cameraChanged(&session, SIGNAL(cameraChanged()));

    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.count(), 1);

    recordOk(session);
    QVERIFY(!QTest::currentTestFailed());
}

void tst_QMediaCaptureSession::recording_stops_when_recorder_removed()
{
    QAudioInput input;
    if (input.device().isNull())
        QSKIP("Recording source not available");

    QMediaRecorder recorder;
    QMediaCaptureSession session;

    QSignalSpy audioInputChanged(&session, SIGNAL(audioInputChanged()));
    QSignalSpy recorderChanged(&session, SIGNAL(recorderChanged()));
    QSignalSpy recorderErrorSignal(&recorder, SIGNAL(errorOccurred(Error, const QString &)));
    QSignalSpy durationChanged(&recorder, SIGNAL(durationChanged(qint64)));

    session.setAudioInput(&input);
    QTRY_COMPARE(audioInputChanged.count(), 1);

    session.setRecorder(&recorder);
    QTRY_COMPARE(recorderChanged.count(), 1);

    recorder.record();
    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::RecordingState);
    QVERIFY(durationChanged.wait(1000));

    session.setRecorder(nullptr);
    QTRY_COMPARE(recorderChanged.count(), 2);

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

    QSignalSpy cameraChanged(&session, SIGNAL(cameraChanged()));
    QSignalSpy imageCaptureChanged(&session, SIGNAL(imageCaptureChanged()));
    QSignalSpy readyForCaptureChanged(&capture, SIGNAL(readyForCaptureChanged(bool)));

    QVERIFY(!capture.isAvailable());
    QVERIFY(!capture.isReadyForCapture());

    session.setImageCapture(&capture);
    QTRY_COMPARE(imageCaptureChanged.count(), 1);
    QVERIFY(!capture.isAvailable());
    QVERIFY(!capture.isReadyForCapture());

    session.setCamera(&camera);
    QTRY_COMPARE(cameraChanged.count(), 1);
    QVERIFY(capture.isAvailable());

    QVERIFY(!capture.isReadyForCapture());

    camera.setActive(true);
    QTRY_COMPARE(readyForCaptureChanged.count(), 1);
    QVERIFY(capture.isReadyForCapture());

    session.setImageCapture(nullptr);
    QTRY_COMPARE(imageCaptureChanged.count(), 2);
    QTRY_COMPARE(readyForCaptureChanged.count(), 2);

    QVERIFY(!capture.isAvailable());
    QVERIFY(!capture.isReadyForCapture());

    session.setImageCapture(&capture);
    QTRY_COMPARE(imageCaptureChanged.count(), 3);
    QTRY_COMPARE(readyForCaptureChanged.count(), 3);
    QVERIFY(capture.isAvailable());
    QVERIFY(capture.isReadyForCapture());
}

void tst_QMediaCaptureSession::capture_is_not_available_when_Camera_is_null()
{
    QCamera camera;

    if (!camera.isAvailable())
        QSKIP("No video input available");

    QImageCapture capture;
    QMediaCaptureSession session;

    QSignalSpy cameraChanged(&session, SIGNAL(cameraChanged()));
    QSignalSpy capturedSignal(&capture, SIGNAL(imageCaptured(int,QImage)));
    QSignalSpy readyForCaptureChanged(&capture, SIGNAL(readyForCaptureChanged(bool)));

    session.setImageCapture(&capture);
    session.setCamera(&camera);
    camera.setActive(true);

    QTRY_COMPARE(readyForCaptureChanged.count(), 1);
    QVERIFY(capture.isReadyForCapture());

    QVERIFY(capture.capture() >= 0);
    QTRY_COMPARE(capturedSignal.count(), 1);

    session.setCamera(nullptr);

    QTRY_COMPARE(readyForCaptureChanged.count(), 2);
    QVERIFY(!capture.isReadyForCapture());
    QVERIFY(!capture.isAvailable());
    QVERIFY(capture.capture() < 0);
}

void tst_QMediaCaptureSession::can_add_ImageCapture_and_capture_during_recording()
{
    QCamera camera;

    if (!camera.isAvailable())
        QSKIP("No video input available");

    QImageCapture capture;
    QMediaCaptureSession session;
    QMediaRecorder recorder;

    QSignalSpy recorderChanged(&session, SIGNAL(recorderChanged()));
    QSignalSpy recorderErrorSignal(&recorder, SIGNAL(errorOccurred(Error, const QString &)));
    QSignalSpy durationChanged(&recorder, SIGNAL(durationChanged(qint64)));
    QSignalSpy imageCaptureChanged(&session, SIGNAL(imageCaptureChanged()));
    QSignalSpy readyForCaptureChanged(&capture, SIGNAL(readyForCaptureChanged(bool)));
    QSignalSpy capturedSignal(&capture, SIGNAL(imageCaptured(int,QImage)));

    session.setCamera(&camera);
    camera.setActive(true);

    session.setRecorder(&recorder);
    QTRY_COMPARE(recorderChanged.count(), 1);

    recorder.record();
    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::RecordingState);
    QVERIFY(durationChanged.wait(1000));

    session.setImageCapture(&capture);
    QTRY_COMPARE(imageCaptureChanged.count(), 1);
    QTRY_COMPARE(readyForCaptureChanged.count(), 1);
    QVERIFY(capture.isReadyForCapture());

    QVERIFY(capture.capture() >= 0);
    QTRY_COMPARE(capturedSignal.count(), 1);

    session.setImageCapture(nullptr);
    QTRY_COMPARE(readyForCaptureChanged.count(), 2);
    QVERIFY(!capture.isReadyForCapture());

    recorder.stop();
    QTRY_VERIFY(recorder.recorderState() == QMediaRecorder::StoppedState);
    QVERIFY(recorderErrorSignal.isEmpty());

    QString fileName = recorder.actualLocation().toLocalFile();
    QVERIFY(!fileName.isEmpty());
    QTRY_VERIFY(QFileInfo(fileName).size() > 0);
    QFile(fileName).remove();
}

QTEST_MAIN(tst_QMediaCaptureSession)

#include "tst_qmediacapturesession.moc"
