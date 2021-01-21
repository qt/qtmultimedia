/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
#include <qmediasource.h>
#include <qmediaservice.h>
#include <qmediaservice.h>
#include <qmediarecordercontrol.h>
#include <qmediarecorder.h>
#include <qmetadatawritercontrol.h>
#include <qaudioencodersettingscontrol.h>
#include <qmediacontainercontrol.h>
#include <qvideoencodersettingscontrol.h>
#include <qaudioformat.h>

#include "mockmediacontainercontrol.h"
#include "mockmetadatawritercontrol.h"
#include "mockmediarecordercontrol.h"
#include "mockmediasource.h"

QT_USE_NAMESPACE

class TestBindableService : public QMediaService
{
    Q_OBJECT
public:
    TestBindableService(QObject *parent, QObject *control):
        QMediaService(parent),
        mockControl(control),
        hasControls(true)
    {
        mockContainerControl = new MockMediaContainerControl(parent); //Creating the object for Media
        mockMetaDataControl = new MockMetaDataWriterControl(parent); //Creating the object for MetaData
    }

    QObject *requestControl(const char *name) override
    {
        if (hasControls && qstrcmp(name,QMediaRecorderControl_iid) == 0)
            return mockControl;
        if (hasControls && qstrcmp(name,QMediaContainerControl_iid) == 0)
            return mockContainerControl;
        if (hasControls && qstrcmp(name, QMetaDataWriterControl_iid) == 0)
            return mockMetaDataControl;

        return nullptr;
    }

    void releaseControl(QObject *) override {}
    //Initialising the objects for the media
    QObject *mockControl;
    QMediaContainerControl *mockContainerControl;
    MockMetaDataWriterControl *mockMetaDataControl;
    bool hasControls;
};

class tst_QMediaSink:public QObject
{
    Q_OBJECT
private slots:
    void init()
    {

    }

    void cleanup()
    {

    }

    void testMediaSource() //Verifying the mediasource api
    {
        MockMediaRecorderControl recorderControl(nullptr);
        TestBindableService service(nullptr, &recorderControl);
        service.mockMetaDataControl->populateMetaData();
        MockMediaSource object(nullptr, &service);
        QMediaRecorder recorder(&object);
        QMediaSource *obj = recorder.mediaSource();
        QVERIFY(obj != nullptr);
        QVERIFY(obj->isAvailable());
    }

    void testDestructor() //Invoking the destructor
    {
        MockMediaRecorderControl recorderControl(nullptr);
        TestBindableService service(nullptr, &recorderControl);
        service.mockMetaDataControl->populateMetaData();
        MockMediaSource object(nullptr, &service);
        QMediaRecorder *recorder = new QMediaRecorder(&object);
        QVERIFY(recorder->isAvailable());
        delete recorder;
        recorder = nullptr;
    }
};

QTEST_MAIN(tst_QMediaSink)
#include "tst_qmediasink.moc"
