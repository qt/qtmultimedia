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
#ifndef TST_QMEDIARECORDER_H
#define TST_QMEDIARECORDER_H

#include <QtTest/QtTest>
#include <QDebug>
#include <qmediaobject.h>
#include <qmediacontrol.h>
#include <qmediaservice.h>
#include <qmediarecordercontrol.h>
#include <qmediarecorder.h>
#include <qmetadatawritercontrol.h>
#include <qaudioendpointselector.h>
#include <qaudioencodercontrol.h>
#include <qmediacontainercontrol.h>
#include <qvideoencodercontrol.h>

#include <qaudioformat.h>

QT_USE_NAMESPACE
class MockMediaContainerControl : public QMediaContainerControl
{
    Q_OBJECT
public:
    MockMediaContainerControl(QObject *parent):
        QMediaContainerControl(parent)
    {
        m_supportedContainers.append("wav");
        m_supportedContainers.append("mp3");
        m_supportedContainers.append("mov");

        m_descriptions.insert("wav", "WAV format");
        m_descriptions.insert("mp3", "MP3 format");
        m_descriptions.insert("mov", "MOV format");
    }

    virtual ~MockMediaContainerControl() {};

    QStringList supportedContainers() const
    {
        return m_supportedContainers;
    }

    QString containerMimeType() const
    {
        return m_format;
    }

    void setContainerMimeType(const QString &formatMimeType)
    {
        if (m_supportedContainers.contains(formatMimeType))
            m_format = formatMimeType;
    }

    QString containerDescription(const QString &formatMimeType) const
    {
        return m_descriptions.value(formatMimeType);
    }

private:
    QStringList m_supportedContainers;
    QMap<QString, QString> m_descriptions;
    QString m_format;
};

class MockVideoEncodeProvider : public QVideoEncoderControl
{
    Q_OBJECT
public:
    MockVideoEncodeProvider(QObject *parent):
        QVideoEncoderControl(parent)
    {
        m_supportedEncodeOptions.insert("video/3gpp", QStringList() << "quantizer" << "me");
        m_supportedEncodeOptions.insert("video/H264", QStringList() << "quantizer" << "me" << "bframes");
        m_videoCodecs << "video/3gpp" << "video/H264";
        m_sizes << QSize(320,240) << QSize(640,480);
        m_framerates << 30 << 15 << 1;
    }
    ~MockVideoEncodeProvider() {}

    QVideoEncoderSettings videoSettings() const { return m_videoSettings; }
    void setVideoSettings(const QVideoEncoderSettings &settings) { m_videoSettings = settings; };

    QList<QSize> supportedResolutions(const QVideoEncoderSettings & = QVideoEncoderSettings(),
                                      bool *continuous = 0) const
    {
        if (continuous)
            *continuous = true;

        return m_sizes;
    }

    QList<qreal> supportedFrameRates(const QVideoEncoderSettings & = QVideoEncoderSettings(),
                                     bool *continuous = 0) const
    {
        if (continuous)
            *continuous = false;

        return m_framerates;
    }

    QStringList supportedVideoCodecs() const { return m_videoCodecs; }
    QString videoCodecDescription(const QString &codecName) const { return codecName; }

    QStringList supportedEncodingOptions(const QString &codec) const
    {
        return m_supportedEncodeOptions.value(codec);
    }

    QVariant encodingOption(const QString &codec, const QString &name) const
    {
        return m_encodeOptions[codec].value(name);
    }

    void setEncodingOption(const QString &codec, const QString &name, const QVariant &value)
    {
        m_encodeOptions[codec][name] = value;
    }

private:
    QVideoEncoderSettings m_videoSettings;

    QMap<QString, QStringList> m_supportedEncodeOptions;
    QMap< QString, QMap<QString, QVariant> > m_encodeOptions;

    QStringList m_videoCodecs;
    QList<QSize> m_sizes;
    QList<qreal> m_framerates;
};

class MockAudioEncodeProvider : public QAudioEncoderControl
{
    Q_OBJECT
public:
    MockAudioEncodeProvider(QObject *parent):
        QAudioEncoderControl(parent)
    {
        m_codecs << "audio/pcm" << "audio/mpeg";
        m_supportedEncodeOptions.insert("audio/pcm", QStringList());
        m_supportedEncodeOptions.insert("audio/mpeg", QStringList() << "quality" << "bitrate" << "mode" << "vbr");
        m_audioSettings.setCodec("audio/pcm");
        m_audioSettings.setBitRate(128*1024);
    }

    ~MockAudioEncodeProvider() {}

    QAudioEncoderSettings audioSettings() const { return m_audioSettings; }
    void setAudioSettings(const QAudioEncoderSettings &settings) { m_audioSettings = settings; }

    QList<int> supportedSampleRates(const QAudioEncoderSettings & = QAudioEncoderSettings(), bool *continuous = 0) const
    {
        if (continuous)
            *continuous = false;

        return QList<int>() << 44100;
    }

    QStringList supportedAudioCodecs() const
    {
        return m_codecs;
    }

    QString codecDescription(const QString &codecName) const
    {
        if (codecName == "audio/pcm")
            return QString("Pulse Code Modulation");

        if (codecName == "audio/mpeg")
            return QString("MP3 audio format");

        return QString();
    }


    QStringList supportedEncodingOptions(const QString &codec) const
    {
        return m_supportedEncodeOptions.value(codec);
    }

    QVariant encodingOption(const QString &codec, const QString &name) const
    {
        return m_encodeOptions[codec].value(name);
    }

    void setEncodingOption(const QString &codec, const QString &name, const QVariant &value)
    {
        m_encodeOptions[codec][name] = value;
    }

private:
    QAudioEncoderSettings m_audioSettings;

    QStringList  m_codecs;
    QStringList  m_codecsDesc;

    QMap<QString, QStringList> m_supportedEncodeOptions;
    QMap< QString, QMap<QString, QVariant> > m_encodeOptions;

};

class MockAudioEndpointSelectorProvider : public QAudioEndpointSelector
{
    Q_OBJECT
public:
    MockAudioEndpointSelectorProvider(QObject *parent):
        QAudioEndpointSelector(parent)
    {
        m_names << "device1" << "device2" << "device3";
        m_descriptions << "dev1 comment" << "dev2 comment" << "dev3 comment";
        emit availableEndpointsChanged();
    }
    ~MockAudioEndpointSelectorProvider() {};

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
    QString         m_audioInput;
    QList<QString>  m_names;
    QList<QString>  m_descriptions;
};

class MockProvider : public QMediaRecorderControl
{
    Q_OBJECT

public:
    MockProvider(QObject *parent):
        QMediaRecorderControl(parent),
    m_state(QMediaRecorder::StoppedState),
    m_position(0),
    m_muted(false) {}

    QUrl outputLocation() const
    {
        return m_sink;
    }

    bool setOutputLocation(const QUrl &sink)
    {
        m_sink = sink;
        return true;
    }

    QMediaRecorder::State state() const
    {
        return m_state;
    }

    qint64 duration() const
    {
        return m_position;
    }

    bool isMuted() const
    {
        return m_muted;
    }

    void applySettings() {}

    using QMediaRecorderControl::error;

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


class QtTestMetaDataProvider : public QMetaDataWriterControl
{
    Q_OBJECT
public:
    QtTestMetaDataProvider(QObject *parent = 0)
        : QMetaDataWriterControl(parent)
        , m_available(false)
        , m_writable(false)
    {
    }

    bool isMetaDataAvailable() const { return m_available; }
    void setMetaDataAvailable(bool available) {
        if (m_available != available)
            emit metaDataAvailableChanged(m_available = available);
    }
    QList<QtMultimediaKit::MetaData> availableMetaData() const { return m_data.keys(); }

    bool isWritable() const { return m_writable; }
    void setWritable(bool writable) { emit writableChanged(m_writable = writable); }

    QVariant metaData(QtMultimediaKit::MetaData key) const { return m_data.value(key); }
    void setMetaData(QtMultimediaKit::MetaData key, const QVariant &value) {
        m_data.insert(key, value); }

    QVariant extendedMetaData(const QString &key) const { return m_extendedData.value(key); }
    void setExtendedMetaData(const QString &key, const QVariant &value) {
        m_extendedData.insert(key, value); }

    QStringList availableExtendedMetaData() const { return m_extendedData.keys(); }

    using QMetaDataWriterControl::metaDataChanged;

    void populateMetaData()
    {
        m_available = true;
    }

    bool m_available;
    bool m_writable;
    QMap<QtMultimediaKit::MetaData, QVariant> m_data;
    QMap<QString, QVariant> m_extendedData;
};

class MockService : public QMediaService
{
    Q_OBJECT
public:
    MockService(QObject *parent, QMediaControl *control):
        QMediaService(parent),
        mockControl(control),
        hasControls(true)
    {
        mockAudioEndpointSelector = new MockAudioEndpointSelectorProvider(parent);
        mockAudioEncodeControl = new MockAudioEncodeProvider(parent);
        mockFormatControl = new MockMediaContainerControl(parent);
        mockVideoEncodeControl = new MockVideoEncodeProvider(parent);
        mockMetaDataControl = new QtTestMetaDataProvider(parent);
    }

    QMediaControl* requestControl(const char *name)
    {
        if(hasControls && qstrcmp(name,QAudioEncoderControl_iid) == 0)
            return mockAudioEncodeControl;
        if(hasControls && qstrcmp(name,QAudioEndpointSelector_iid) == 0)
            return mockAudioEndpointSelector;
        if(hasControls && qstrcmp(name,QMediaRecorderControl_iid) == 0)
            return mockControl;
        if(hasControls && qstrcmp(name,QMediaContainerControl_iid) == 0)
            return mockFormatControl;
        if(hasControls && qstrcmp(name,QVideoEncoderControl_iid) == 0)
            return mockVideoEncodeControl;
        if (hasControls && qstrcmp(name, QMetaDataWriterControl_iid) == 0)
            return mockMetaDataControl;

        return 0;
    }

    void releaseControl(QMediaControl*) {}

    QMediaControl   *mockControl;
    QAudioEndpointSelector  *mockAudioEndpointSelector;
    QAudioEncoderControl    *mockAudioEncodeControl;
    QMediaContainerControl     *mockFormatControl;
    QVideoEncoderControl    *mockVideoEncodeControl;
    QtTestMetaDataProvider *mockMetaDataControl;
    bool hasControls;
};

class MockObject : public QMediaObject
{
    Q_OBJECT
public:
    MockObject(QObject *parent, MockService *service):
        QMediaObject(parent, service)
    {
    }
};

class tst_QMediaRecorder: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void testNullService();
    void testNullControls();
    void testError();
    void testSink();
    void testRecord();
    void testMute();
    void testAudioDeviceControl();
    void testAudioEncodeControl();
    void testMediaFormatsControl();
    void testVideoEncodeControl();
    void testEncodingSettings();
    void testAudioSettings();
    void testVideoSettings();

    void nullMetaDataControl();
    void isMetaDataAvailable();
    void isWritable();
    void metaDataChanged();
    void metaData_data();
    void metaData();
    void setMetaData_data();
    void setMetaData();
    void extendedMetaData_data() { metaData_data(); }
    void extendedMetaData();
    void setExtendedMetaData_data() { extendedMetaData_data(); }
    void setExtendedMetaData();

private:
    QAudioEncoderControl* encode;
    QAudioEndpointSelector* audio;
    MockObject      *object;
    MockService		*service;
    MockProvider    *mock;
    QMediaRecorder  *capture;
    QVideoEncoderControl* videoEncode;
};
#endif //TST_QMEDIARECORDER_H
