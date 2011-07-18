/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
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

#include <QtTest/QtTest>
#include <QDebug>

#include <qaudioformat.h>

#include <qaudiocapturesource.h>
#include <qaudioencodercontrol.h>
#include <qmediarecordercontrol.h>
#include <qaudioendpointselector.h>

//TESTED_COMPONENT=src/multimedia

QT_USE_NAMESPACE
class MockAudioEncoderControl : public QAudioEncoderControl
{
    Q_OBJECT
public:
    MockAudioEncoderControl(QObject *parent = 0):
            QAudioEncoderControl(parent)
    {
        m_codecs << "audio/pcm" << "audio/mpeg";
        m_descriptions << "Pulse Code Modulation" << "mp3 format";
        m_audioSettings.setCodec("audio/pcm");
        m_audioSettings.setSampleRate(8000);
        m_freqs << 8000 << 11025 << 22050 << 44100;
    }

    ~MockAudioEncoderControl() {}

    QStringList supportedAudioCodecs() const { return m_codecs; }
    QString codecDescription(const QString &codecName) const { return m_descriptions.at(m_codecs.indexOf(codecName)); }

    QStringList supportedEncodingOptions(const QString &) const { return QStringList() << "bitrate"; }
    QVariant encodingOption(const QString &, const QString &) const { return m_optionValue; }
    void setEncodingOption(const QString &, const QString &, const QVariant &value) { m_optionValue = value; }

    QList<int> supportedSampleRates(const QAudioEncoderSettings & = QAudioEncoderSettings(),
                                    bool *continuous = 0) const
    {
        if (continuous)
            *continuous = false;
        return m_freqs;
    }
    QList<int> supportedChannelCounts(const QAudioEncoderSettings & = QAudioEncoderSettings()) const { QList<int> list; list << 1 << 2; return list; }

    QAudioEncoderSettings audioSettings() const { return m_audioSettings; }
    void setAudioSettings(const QAudioEncoderSettings &settings) { m_audioSettings = settings;}

    QStringList m_codecs;
    QStringList m_descriptions;

    QAudioEncoderSettings m_audioSettings;

    QList<int> m_freqs;
    QVariant m_optionValue;
};

class MockMediaRecorderControl : public QMediaRecorderControl
{
    Q_OBJECT
public:
    MockMediaRecorderControl(QObject *parent = 0):
            QMediaRecorderControl(parent),
            m_state(QMediaRecorder::StoppedState),
            m_position(0),
            m_muted(false) {}

    ~MockMediaRecorderControl() {}

    QUrl outputLocation() const { return m_sink; }
    bool setOutputLocation(const QUrl &sink) { m_sink = sink; return true; }
    QMediaRecorder::State state() const { return m_state; }
    qint64 duration() const { return m_position; }
    void applySettings() {}
    bool isMuted() const { return m_muted; }

public slots:
    void record()
    {
        m_state = QMediaRecorder::RecordingState;
        m_position=1;
        emit stateChanged(m_state);
        emit durationChanged(m_position);
    }
    void pause()
    {
        m_state = QMediaRecorder::PausedState;
        emit stateChanged(m_state);
    }

    void stop()
    {
        m_position=0;
        m_state = QMediaRecorder::StoppedState;
        emit stateChanged(m_state);
    }

    void setMuted(bool muted)
    {
        if (m_muted != muted)
            emit mutedChanged(m_muted = muted);
    }

public:
    QUrl       m_sink;
    QMediaRecorder::State m_state;
    qint64     m_position;
    bool m_muted;
};

class MockAudioEndpointSelector : public QAudioEndpointSelector
{
    Q_OBJECT
public:
    MockAudioEndpointSelector(QObject *parent):
        QAudioEndpointSelector(parent)
    {
        m_names << "device1" << "device2" << "device3";
        m_descriptions << "dev1 comment" << "dev2 comment" << "dev3 comment";
        m_audioInput = "device1";
        emit availableEndpointsChanged();
    }
    ~MockAudioEndpointSelector() {};

    QList<QString> availableEndpoints() const
    {
        return m_names;
    }

    QString endpointDescription(const QString& name) const
    {
        QString desc;

        for(int i = 0; i < m_names.count(); i++) {
            if (m_names.at(i).compare(name) == 0) {
                desc = m_descriptions.at(i);
                break;
            }
        }
        return desc;
    }

    QString defaultEndpoint() const
    {
        return m_names.at(0);
    }

    QString activeEndpoint() const
    {
        return m_audioInput;
    }

public Q_SLOTS:
    void setActiveEndpoint(const QString& name)
    {
        m_audioInput = name;
        emit activeEndpointChanged(name);
    }

private:
    QString        m_audioInput;
    QList<QString> m_names;
    QList<QString> m_descriptions;
};


class MockAudioSourceService : public QMediaService
{
    Q_OBJECT

public:
    MockAudioSourceService(): QMediaService(0), hasAudioDeviceControl(true)
    {
        mockAudioEncoderControl = new MockAudioEncoderControl(this);
        mockMediaRecorderControl = new MockMediaRecorderControl(this);
        mockAudioEndpointSelector = new MockAudioEndpointSelector(this);
    }

    ~MockAudioSourceService()
    {
        delete mockAudioEncoderControl;
        delete mockMediaRecorderControl;
        delete mockAudioEndpointSelector;
    }

    QMediaControl* requestControl(const char *iid)
    {
        if (qstrcmp(iid, QAudioEncoderControl_iid) == 0)
            return mockAudioEncoderControl;

        if (qstrcmp(iid, QMediaRecorderControl_iid) == 0)
            return mockMediaRecorderControl;

        if (hasAudioDeviceControl && qstrcmp(iid, QAudioEndpointSelector_iid) == 0)
            return mockAudioEndpointSelector;

        return 0;
    }

    void releaseControl(QMediaControl*) {}

    MockAudioEncoderControl *mockAudioEncoderControl;
    MockMediaRecorderControl *mockMediaRecorderControl;
    MockAudioEndpointSelector *mockAudioEndpointSelector;
    bool hasAudioDeviceControl;
};

class MockProvider : public QMediaServiceProvider
{
public:
    MockProvider(MockAudioSourceService *service):mockService(service) {}
    QMediaService *requestService(const QByteArray&, const QMediaServiceProviderHint &)
    {
        return mockService;
    }

    void releaseService(QMediaService *) {}

    MockAudioSourceService *mockService;
};


class tst_QAudioCaptureSource: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    //void testNullService();
    //void testNullControl();
    void testAudioSource();
    void testOptions();
    void testDevices();

private:
    QAudioCaptureSource *audiosource;
    MockAudioSourceService  *mockAudioSourceService;
    MockProvider *mockProvider;
};

void tst_QAudioCaptureSource::initTestCase()
{
    mockAudioSourceService = new MockAudioSourceService;
    mockProvider = new MockProvider(mockAudioSourceService);
}

void tst_QAudioCaptureSource::cleanupTestCase()
{
    delete audiosource;
    delete mockProvider;
}
/*
void tst_QAudioCaptureSource::testNullService()
{
    MockProvider provider(0);
    QAudioCaptureSource source(0, &provider);

    QCOMPARE(source.audioInputs().size(), 0);
    QCOMPARE(source.defaultAudioInput(), QString());
    QCOMPARE(source.activeAudioInput(), QString());
}
*/
/*
void tst_QAudioCaptureSource::testNullControl()
{
    MockAudioSourceService service;
    service.hasAudioDeviceControl = false;
    MockProvider provider(&service);
    QAudioCaptureSource source(0, &provider);

    QCOMPARE(source.audioInputs().size(), 0);
    QCOMPARE(source.defaultAudioInput(), QString());
    QCOMPARE(source.activeAudioInput(), QString());

    QCOMPARE(source.audioDescription("blah"), QString());

    QSignalSpy deviceNameSpy(&source, SIGNAL(activeAudioInputChanged(QString)));

    source.setAudioInput("blah");
    QCOMPARE(deviceNameSpy.count(), 0);
}
*/
void tst_QAudioCaptureSource::testAudioSource()
{
    audiosource = new QAudioCaptureSource(0, mockProvider);

    QCOMPARE(audiosource->service(),(QMediaService *) mockAudioSourceService);
}

void tst_QAudioCaptureSource::testOptions()
{
    const QString codec(QLatin1String("mp3"));

    QStringList options = mockAudioSourceService->mockAudioEncoderControl->supportedEncodingOptions(codec);
    QVERIFY(options.count() == 1);
    mockAudioSourceService->mockAudioEncoderControl->setEncodingOption(codec, options.first(),8000);
    QVERIFY(mockAudioSourceService->mockAudioEncoderControl->encodingOption(codec, options.first()).toInt() == 8000);
}

void tst_QAudioCaptureSource::testDevices()
{
    QList<QString> devices = audiosource->audioInputs();
    QVERIFY(devices.size() > 0);
    QVERIFY(devices.at(0).compare("device1") == 0);
    QVERIFY(audiosource->audioDescription("device1").compare("dev1 comment") == 0);
    QVERIFY(audiosource->defaultAudioInput() == "device1");

    QSignalSpy checkSignal(audiosource, SIGNAL(activeAudioInputChanged(QString)));
    audiosource->setAudioInput("device2");
    QVERIFY(audiosource->activeAudioInput().compare("device2") == 0);
    QVERIFY(checkSignal.count() == 1);
}

QTEST_MAIN(tst_QAudioCaptureSource)

#include "tst_qaudiocapturesource.moc"
