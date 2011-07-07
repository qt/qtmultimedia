/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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

#ifndef S60AUDIOCAPTURESESSION_H
#define S60AUDIOCAPTURESESSION_H

#include <qmobilityglobal.h>
#include <QtCore/qobject.h>
#include <QFile>
#include <QUrl>
#include <QList>
#include <QHash>
#include <QMap>
#include "qaudioformat.h"
#include <qmediarecorder.h>

#include <mda/common/audio.h>
#include <mda/common/resource.h>
#include <mda/client/utility.h>
#include <mdaaudiosampleeditor.h>
#include <mmf/common/mmfutilities.h>

#ifdef AUDIOINPUT_ROUTING
#include <audioinput.h>
#endif //AUDIOINPUT_ROUTING

QT_BEGIN_NAMESPACE
struct ControllerData
{
	int controllerUid;
	int destinationFormatUid;
	QString destinationFormatDescription;
	QString fileExtension;
};

struct CodecData
{
    TFourCC fourCC;
    QString codecDescription;
};
QT_END_NAMESPACE

QT_USE_NAMESPACE

class S60AudioCaptureSession : public QObject, public MMdaObjectStateChangeObserver
{
    Q_OBJECT
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_ENUMS(TAudioCaptureState)
public:

    enum TAudioCaptureState
    {
        ENotInitialized = 0,
        EInitialized,
        EOpenCompelete,
        ERecording,
        EPaused,
        ERecordComplete
    };

    S60AudioCaptureSession(QObject *parent = 0);
    ~S60AudioCaptureSession();

    QAudioFormat format() const;
    bool setFormat(const QAudioFormat &format);
    QAudioEncoderSettings settings() const;
    bool setEncoderSettings(const QAudioEncoderSettings &setting);
    QStringList supportedAudioCodecs() const;
    QString codecDescription(const QString &codecName);
    bool setAudioCodec(const QString &codecName);
    QString audioCodec() const;
    QString audioContainer() const;
    QStringList supportedAudioContainers() const;
    bool setAudioContainer(const QString &containerMimeType);
    QString audioContainerDescription(const QString &containerName);
    QList<int> supportedAudioSampleRates(const QAudioEncoderSettings &settings) const;
    QUrl outputLocation() const;
    bool setOutputLocation(const QUrl& sink);
    qint64 position() const;
    void record();
    void pause();
    void stop();
    void mute(bool muted);
    bool muted();

    QString activeEndpoint() const;
    QString defaultEndpoint() const;
    QList<QString> availableEndpoints() const;
    QString endpointDescription(const QString& name) const;

#ifdef AUDIOINPUT_ROUTING
    static const QString microPhone;
    static const QString voiceCall;
    static const QString fmRadio;
#endif //AUDIOINPUT_ROUTING
private:
    void initializeSessionL();
    void setError(TInt aError);
    QMediaRecorder::Error fromSymbianErrorToMultimediaError(int error);
    void prepareSinkL();
    void updateAudioContainersL();
    void populateAudioCodecsDataL();
    void retrieveSupportedAudioSampleRatesL();
    void applyAudioSettingsL();
    TFourCC determinePCMFormat();
    void setDefaultSettings();
    // MMdaObjectStateChangeObserver
    void MoscoStateChangeEvent(CBase* aObject, TInt aPreviousState,
            TInt aCurrentState, TInt aErrorCode);
    void MoscoStateChangeEventL(CBase* aObject, TInt aPreviousState,
            TInt aCurrentState, TInt aErrorCode);

#ifdef AUDIOINPUT_ROUTING
    QString qStringFromTAudioInputPreference(CAudioInput::TAudioInputPreference input) const;
    void initAudioInputs();
    void doSetAudioInputL(const QString& name);
#endif //AUDIOINPUT_ROUTING

public Q_SLOTS:
    void setActiveEndpoint(const QString& audioEndpoint);


Q_SIGNALS:
    void stateChanged(S60AudioCaptureSession::TAudioCaptureState);
    void positionChanged(qint64 position);
    void error(int error, const QString &errorString);
    void activeEndpointChanged(const QString &audioEndpoint);
private:
    QString m_container;
    QUrl m_sink;
    TTimeIntervalMicroSeconds m_pausedPosition;
    CMdaAudioRecorderUtility *m_recorderUtility;
    TAudioCaptureState m_captureState;
    QAudioFormat m_format;
    QAudioEncoderSettings m_audioEncoderSettings;
    QHash<QString, ControllerData> m_controllerIdMap;
    QHash<QString, CodecData>  m_audioCodeclist;
    QList<int> m_supportedSampleRates;
    int m_error;
    bool m_isMuted;
    RFs m_fsSession;

#ifdef AUDIOINPUT_ROUTING
    bool m_setActiveEndPoint;
    CAudioInput *m_audioInput;

#endif //AUDIOINPUT_ROUTING
    QMap<QString, QString> m_audioInputs;
    QString m_audioEndpoint;


};

#endif // S60AUDIOCAPTURESESSION_H
