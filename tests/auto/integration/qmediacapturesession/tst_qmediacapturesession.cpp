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

#include <qcamera.h>
#include <qcameradevice.h>
#include <qimagecapture.h>
#include <qmediacapturesession.h>
#include <qmediadevices.h>
#include <qmediarecorder.h>
#include <qaudiooutput.h>
#include <qaudioinput.h>
#include <qaudiodevice.h>


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
    void testAudioInputAddRemove();
    void testAudioInputAddRemoveDuringRecording();
};

void tst_QMediaCaptureSession::testAudioInputAddRemove()
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

void tst_QMediaCaptureSession::testAudioInputAddRemoveDuringRecording()
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
    QVERIFY(QFileInfo(fileName).size() > 0);
    QFile(fileName).remove();
}

QTEST_MAIN(tst_QMediaCaptureSession)

#include "tst_qmediacapturesession.moc"
