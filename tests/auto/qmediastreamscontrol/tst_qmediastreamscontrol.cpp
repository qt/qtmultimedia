/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

//TESTED_COMPONENT=src/multimedia

#include <QtCore/QString>
#include <QtTest/QtTest>
#include <QtCore/QCoreApplication>


#include <qmediaplayercontrol.h>
#include <qmediaservice.h>

#include <qmediastreamscontrol.h>

#include <QtGui/QImage>
#include <QtCore/QPointer>

QT_USE_NAMESPACE


#define WAIT_FOR_CONDITION(a,e)            \
    for (int _i = 0; _i < 500; _i += 1) {  \
    if ((a) == (e)) break;             \
    QTest::qWait(10);}

class tst_qmediastreamscontrol : public QObject
{
    Q_OBJECT

public:
    tst_qmediastreamscontrol();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void control_iid();
    void control();
    void isActive();
    void streamCount();
    void streamsChanged();
    void metadata();
};



class mediaStatusList : public QObject, public QList<QMediaStreamsControl::StreamType>
{
    Q_OBJECT
public slots:
    void mediaStatus(QMediaStreamsControl::StreamType status) {
        append(status);
    }

public:
    mediaStatusList(QObject *obj, const char *aSignal)
        : QObject()
    {
        QObject::connect(obj, aSignal, this, SLOT(mediaStatus(QMediaStreamsControl::StreamType)));
    }
};

class QtTestMediaStreamsControl: public QMediaStreamsControl
{
public:
    QtTestMediaStreamsControl(QObject *parent = 0)
        : QMediaStreamsControl(parent)
    {
    }

    int streamCount()
    {
        QList <StreamType> m_stype;

        return streams.count();
    }
    void setStreamCount(int count)
    {
        streams.resize(count);
    }

    StreamType streamType(int index)
    {
        return streams.at(index).type;
    }
    void setStreamType(int index, StreamType type)
    {
        streams[index].type = type;
    }

    QVariant metaData(int index, QtMultimedia::MetaData key)
    {
        QtMultimedia::MetaData keys = key;
        return keys;
    }

    void setMetaData(int index, QtMultimedia::MetaData key, const QVariant &value)
    {
        streams[index].metaData.insert(key, value);
    }

    bool isActive(int index)
    {
        return streams.at(index).active;
    }
    void setActive(int index, bool state)
    {
        streams[index].active = state;
    }

    void setAudioOnlyContent()
    {
        mediaContent = audioOnlyContent;

        m_player->setMedia(*mediaContent);
    }

    void setVideoOnlyContent()
    {
        mediaContent = videoOnlyContent;
        duration = 60000;

        m_player->setMedia(*mediaContent);
    }

    void setAudioVideoContent()
    {
        if (mediaContent == audioVideoContent)
        {
            mediaContent = audioVideoAltContent;
            duration = 101840;
        }
        else
        {
            mediaContent = audioVideoContent;
            duration = 141000;
        }

        m_player->setMedia(*mediaContent);
    }

    void setStreamingContent()
    {
        mediaContent = streamingContent;

        m_player->setMedia(*mediaContent);
    }



public:
    struct Stream
    {
        Stream() : type(UnknownStream), active(false) {}
        StreamType type;
        QMap<QtMultimedia::MetaData, QVariant> metaData;
        bool active;
    };

    QVector<Stream> streams;
    QMediaContent* audioOnlyContent;
    QMediaContent* videoOnlyContent;
    QMediaContent* audioVideoContent;
    QMediaContent* audioVideoAltContent;
    QMediaContent* mediaContent;
    QMediaContent* streamingContent;

    qint64 duration;
    QMediaPlayer *m_player;
    QVideoWidget *m_widget;
    QWidget *m_windowWidget;


};

class QTestMediaStreamsControlA : public QMediaControl
{
    Q_OBJECT
};

#define QTestMediaStreamsControlA_iid "com.nokia.QTestMediaStreamsControlA"
Q_MEDIA_DECLARE_CONTROL(QTestMediaStreamsControlA, QTestMediaStreamsControlA_iid)

class QTestMediaStreamsControlB : public QMediaControl
{
    Q_OBJECT
public:
    QTestMediaStreamsControlB()
        : QMediaControl(0)
        ,ctrlA(0)
        ,ctrlB(0)
        ,ctrlC(0) {}

    bool isActive(int stream)
    {
        return 1;
    }

    int ctrlA;
    int ctrlB;
    int ctrlC;
};

#define QTestMediaStreamsControlB_iid "com.nokia.QTestMediaStreamsControlB"
Q_MEDIA_DECLARE_CONTROL(QTestMediaStreamsControlB, QTestMediaStreamsControlB_iid)


class QTestMediaStreamsControlC : public QMediaControl
{
    Q_OBJECT
};

#define QTestMediaStreamsControlC_iid "com.nokia.QTestMediaStreamsControlC"
Q_MEDIA_DECLARE_CONTROL(QTestMediaStreamsControlC, QTestMediaStreamsControlC_iid) // Yes A.

class QTestMediaStreamsControlD : public QMediaControl
{
    Q_OBJECT
};

#define QTestMediaStreamsControlD_iid "com.nokia.QTestMediaStreamsControlD"
Q_MEDIA_DECLARE_CONTROL(QTestMediaStreamsControlD, QTestMediaStreamsControlD_iid)


class QtTestMediaService : public QMediaService
{
    Q_OBJECT
public:
    QtTestMediaService()
        : QMediaService(0)
        , refA(0)
        , refB(0)
        , refC(0)
    {
    }

    QMediaControl *requestControl(const char *name)
    {
        if (strcmp(name, QTestMediaStreamsControlA_iid) == 0) {
            refA += 1;

            return &controlA;
        } else if (strcmp(name, QTestMediaStreamsControlB_iid) == 0) {
            refB += 1;

            return &controlB;
        } else if (strcmp(name, QTestMediaStreamsControlC_iid) == 0) {
            refA += 1;

            return &controlA;
        } else {
            return 0;
        }
    }

    void releaseControl(QMediaControl *control)
    {
        if (control == &controlA)
            refA -= 1;
        else if (control == &controlB)
            refB -= 1;
        else if (control == &controlC)
            refC -= 1;
    }

    using QMediaService::requestControl;

    int refA;
    int refB;
    int refC;
    QTestMediaStreamsControlA controlA;
    QTestMediaStreamsControlB controlB;
    QTestMediaStreamsControlC controlC;
};


tst_qmediastreamscontrol::tst_qmediastreamscontrol()
{
}

void tst_qmediastreamscontrol::initTestCase()
{
}

void tst_qmediastreamscontrol::cleanupTestCase()
{
}

void tst_qmediastreamscontrol::control_iid()
{

    // Default implementation.
    QCOMPARE(qmediacontrol_iid<QTestMediaStreamsControlA *>(), QTestMediaStreamsControlA_iid);

    // Partial template.
    QVERIFY(qstrcmp(qmediacontrol_iid<QTestMediaStreamsControlA *>(), QTestMediaStreamsControlA_iid) == 0);
}

void tst_qmediastreamscontrol::control()
{
    QtTestMediaService *service = new QtTestMediaService();
    QMediaStreamsControl *control = qobject_cast<QMediaStreamsControl *>
            (service->requestControl("com.nokia.Qt.MediaStreamsControl/1.0"));
    //    QCOMPARE(control,service->controlA.objectName());
    QTestMediaStreamsControlA *controlA = (QTestMediaStreamsControlA *)service->requestControl("controlA");
    //    QCOMPARE(controlA,service->controlA);
    QVERIFY(service->requestControl<QTestMediaStreamsControlA *>());

    service->releaseControl(controlA);
    delete service;
}

void tst_qmediastreamscontrol::isActive()
{
    QTestMediaStreamsControlB ser;
    QVERIFY(ser.isActive(1));
    QtTestMediaStreamsControl m_active;
    //setActive
    m_active.setActive(1,1);
    QVERIFY(m_active.isActive(1));
    //set InActive
    m_active.setActive(2,0);
    QVERIFY(!m_active.isActive(0));
}

//Returns the number of media streams.
void tst_qmediastreamscontrol::streamCount()
{
    QtTestMediaStreamsControl m_cnt;
    m_cnt.setStreamType(0,QMediaStreamsControl::UnknownStream);
    m_cnt.setStreamType(1,QMediaStreamsControl::VideoStream);
    m_cnt.setStreamType(2,QMediaStreamsControl::AudioStream);
    m_cnt.setStreamType(3,QMediaStreamsControl::SubPictureStream);
    m_cnt.setStreamType(4,QMediaStreamsControl::DataStream);
    m_cnt.setStreamCount(5);
    QVERIFY(m_cnt.streamCount() == m_cnt.streams.count());
}

//The signal is emitted when the available streams list is changed.
void tst_qmediastreamscontrol::streamsChanged()
{
    QMediaPlayer *m_player = new QMediaPlayer(0);
    QMediaStreamsControl* m_streamControl = (QMediaStreamsControl*)
            (m_player->service()->requestControl(QTestMediaStreamsControlA_iid));

    QMediaContent videoOnlyContent;

    m_player->setMedia(videoOnlyContent);
    if (m_streamControl) {
        QSignalSpy m_strm_lst_chgSpy(m_streamControl,SIGNAL(streamsChanged()));
        QVERIFY(m_strm_lst_chgSpy.isValid());
        QVERIFY(m_strm_lst_chgSpy.isEmpty());
        WAIT_FOR_CONDITION(m_player->mediaStatus(),QMediaPlayer::LoadedMedia);
        QVERIFY(m_streamControl->streamCount() == 1);
        QVERIFY(m_strm_lst_chgSpy.count() == 1);
    }

    delete m_player;
    m_player = NULL;
}

void tst_qmediastreamscontrol::metadata()
{
    QtTestMediaStreamsControl m_metadata;
    m_metadata.metaData(1,QtMultimedia::AlbumArtist);
    qDebug() << m_metadata.metaData(1,QtMultimedia::AlbumArtist);
}
QTEST_MAIN(tst_qmediastreamscontrol);

#include "tst_qmediastreamscontrol.moc"
