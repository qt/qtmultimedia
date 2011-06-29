/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "DebugMacros.h"

#include "s60audiocapturesession.h"
#include <QtCore/qdebug.h>
#include <QtCore/qurl.h>
#include <QDir>

#include <mda/common/audio.h>
#include <mda/common/resource.h>
#include <mda/client/utility.h>
#include <mdaaudiosampleeditor.h>
#include <mmf/common/mmfcontrollerpluginresolver.h>
#include <mmf/common/mmfcontroller.h>
#include <badesca.h>
#include <bautils.h>
#include <f32file.h>

#ifdef AUDIOINPUT_ROUTING
const QString S60AudioCaptureSession::microPhone("Microphone");
const QString S60AudioCaptureSession::voiceCall("Voice Call");
const QString S60AudioCaptureSession::fmRadio("FM Radio");
#endif

S60AudioCaptureSession::S60AudioCaptureSession(QObject *parent):
    QObject(parent)
    , m_recorderUtility(NULL)
    , m_captureState(ENotInitialized)
    , m_controllerIdMap(QHash<QString, ControllerData>())
    , m_audioCodeclist(QHash<QString, CodecData>())
    , m_error(QMediaRecorder::NoError)
    , m_isMuted(false)
{
   DP0("S60AudioCaptureSession::S60AudioCaptureSession +++");
#ifdef AUDIOINPUT_ROUTING
    m_audioInput = NULL;
    m_setActiveEndPoint = FALSE;
    m_audioEndpoint = S60AudioCaptureSession::microPhone;
#endif //AUDIOINPUT_ROUTING
    TRAPD(err, initializeSessionL());
    setError(err);

    DP0("S60AudioCaptureSession::S60AudioCaptureSession ---");
}

void S60AudioCaptureSession::initializeSessionL()
{
    DP0("S60AudioCaptureSession::initializeSessionL +++");

    m_recorderUtility = CMdaAudioRecorderUtility::NewL(*this, 0, EMdaPriorityNormal, EMdaPriorityPreferenceTimeAndQuality);
    updateAudioContainersL();
    populateAudioCodecsDataL();
    setDefaultSettings();
#ifdef AUDIOINPUT_ROUTING
    initAudioInputs();
#endif
    User::LeaveIfError(m_fsSession.Connect());
    m_captureState = EInitialized;
    emit stateChanged(m_captureState);

    DP0("S60AudioCaptureSession::initializeSessionL ---");
}

void S60AudioCaptureSession::setError(TInt aError)
{
    DP0("S60AudioCaptureSession::setError +++");

    DP1("S60AudioCaptureSession::setError:", aError);

    if (aError == KErrNone)
        return;

    m_error = aError;
    QMediaRecorder::Error recorderError = fromSymbianErrorToMultimediaError(m_error);

    // TODO: fix to user friendly string at some point
    // These error string are only dev usable
    QString symbianError;
    symbianError.append("Symbian:");
    symbianError.append(QString::number(m_error));
    stop();
    emit error(recorderError, symbianError);

    DP0("S60AudioCaptureSession::setError ---");
}

QMediaRecorder::Error S60AudioCaptureSession::fromSymbianErrorToMultimediaError(int error)
{
    DP0("S60AudioCaptureSession::fromSymbianErrorToMultimediaError +++");

    DP1("S60AudioCaptureSession::fromSymbianErrorToMultimediaError:", error);

    switch(error) {
    case KErrNoMemory:
    case KErrNotFound:
    case KErrBadHandle:
    case KErrAbort:
    case KErrCorrupt:
    case KErrGeneral:
    case KErrPathNotFound:
    case KErrUnknown:
    case KErrNotReady:
    case KErrInUse:
    case KErrAccessDenied:
    case KErrLocked:
    case KErrPermissionDenied:
    case KErrAlreadyExists:
        return QMediaRecorder::ResourceError;
    case KErrNotSupported:
    case KErrArgument:
        return QMediaRecorder::FormatError;
    case KErrNone:
    default:
        DP0("S60AudioCaptureSession::fromSymbianErrorToMultimediaError: ---");
        return QMediaRecorder::NoError;
    }
}

S60AudioCaptureSession::~S60AudioCaptureSession()
{
    DP0("S60AudioCaptureSession::~S60AudioCaptureSession +++");
    //stop the utility before deleting it
    stop();
    if (m_recorderUtility)
        delete m_recorderUtility;
    m_fsSession.Close();
    DP0("S60AudioCaptureSession::~S60AudioCaptureSession ---");
}

QAudioFormat S60AudioCaptureSession::format() const
{
    DP0("S60AudioCaptureSession::format");

    return m_format;
}

bool S60AudioCaptureSession::setFormat(const QAudioFormat &format)
{
    DP0("S60AudioCaptureSession::setFormat +++");

    m_format = format;

    DP0("S60AudioCaptureSession::setFormat ---");

    return true;
}

QAudioEncoderSettings S60AudioCaptureSession::settings() const
{
    DP0("S60AudioCaptureSession::settings");

    return m_audioEncoderSettings;
}

bool S60AudioCaptureSession::setEncoderSettings(const QAudioEncoderSettings &setting)
{
    DP0("S60AudioCaptureSession::setEncoderSettings +++");

    m_audioEncoderSettings = setting;

    DP0("S60AudioCaptureSession::setEncoderSettings ---");

    return true;
}

QStringList S60AudioCaptureSession::supportedAudioCodecs() const
{
    DP0("S60AudioCaptureSession::supportedAudioCodecs");

    return m_audioCodeclist.keys();
}

QStringList S60AudioCaptureSession::supportedAudioContainers() const
{
    DP0("S60AudioCaptureSession::supportedAudioContainers");

    return m_controllerIdMap.keys();
}

QString S60AudioCaptureSession::codecDescription(const QString &codecName)
{
    DP0("S60AudioCaptureSession::codecDescription +++");

    if (m_audioCodeclist.keys().contains(codecName)) {

        DP0("S60AudioCaptureSession::codecDescription ---");
        return m_audioCodeclist.value(codecName).codecDescription;
    }
    else {
        DP0("S60AudioCaptureSession::codecDescription ---");

        return QString();
    }
}

QString S60AudioCaptureSession::audioContainerDescription(const QString &containerName)
{
    DP0("S60AudioCaptureSession::audioContainerDescription +++");

    if (m_controllerIdMap.keys().contains(containerName)) {
        DP0("S60AudioCaptureSession::audioContainerDescription ---");

        return m_controllerIdMap.value(containerName).destinationFormatDescription;
    }
    else {
        DP0("S60AudioCaptureSession::audioContainerDescription ---");

        return QString();
    }
}

bool S60AudioCaptureSession::setAudioCodec(const QString &codecName)
{
    DP0("S60AudioCaptureSession::setAudioCodec");

    QStringList codecs = supportedAudioCodecs();
    if(codecs.contains(codecName)) {
        m_format.setCodec(codecName);
        return true;
    }
    return false;
}

bool S60AudioCaptureSession::setAudioContainer(const QString &containerMimeType)
{
    DP0("S60AudioCaptureSession::setAudioContainer");

    QStringList containers = supportedAudioContainers();
    if (containerMimeType == "audio/mpeg")
        {
        m_container = "audio/mp4";
        return true;
        }
    if(containers.contains(containerMimeType)) {
        m_container = containerMimeType;
        return true;
    }
    return false;
}

QString S60AudioCaptureSession::audioCodec() const
{
    DP0("S60AudioCaptureSession::audioCodec");

    return m_format.codec();
}

QString S60AudioCaptureSession::audioContainer() const
{
    DP0("S60AudioCaptureSession::audioContainer");

    return m_container;
}

QUrl S60AudioCaptureSession::outputLocation() const
{
    DP0("S60AudioCaptureSession::outputLocation");

    return m_sink;
}

bool S60AudioCaptureSession::setOutputLocation(const QUrl& sink)
{
    DP0("S60AudioCaptureSession::setOutputLocation");

    QString filename = QDir::toNativeSeparators(sink.toString());
    TPtrC16 path(reinterpret_cast<const TUint16*>(filename.utf16()));
    TRAPD(err, BaflUtils::EnsurePathExistsL(m_fsSession,path));
    if (err == KErrNone) {
        m_sink = sink;
        setError(err);
        return true;
    }else {
        setError(err);
        return false;
    }
}

qint64 S60AudioCaptureSession::position() const
{
    DP0("S60AudioCaptureSession::position");

    if ((m_captureState != ERecording) || !m_recorderUtility)
        return 0;

    return m_recorderUtility->Duration().Int64() / 1000;
}

void S60AudioCaptureSession::prepareSinkL()
{
    DP0("S60AudioCaptureSession::prepareSinkL +++");

    /* If m_outputLocation is null, set a default location */
    if (m_sink.isEmpty()) {
        QDir outputDir(QDir::rootPath());
        int lastImage = 0;
        int fileCount = 0;
        foreach(QString fileName, outputDir.entryList(QStringList() << "recordclip_*")) {
            int imgNumber = fileName.mid(5, fileName.size() - 9).toInt();
            lastImage = qMax(lastImage, imgNumber);
            if (outputDir.exists(fileName))
                fileCount += 1;
        }
        lastImage += fileCount;
        m_sink = QUrl(QDir::toNativeSeparators(outputDir.canonicalPath() + QString("/recordclip_%1").arg(lastImage + 1, 4, 10, QLatin1Char('0'))));
    }

    QString sink = QDir::toNativeSeparators(m_sink.toString());
    TPtrC16 path(reinterpret_cast<const TUint16*>(sink.utf16()));
    if (BaflUtils::FileExists(m_fsSession, path))
            BaflUtils::DeleteFile(m_fsSession, path);

    int index = sink.lastIndexOf('.');
    if (index != -1)
        sink.chop(sink.length()-index);

    sink.append(m_controllerIdMap.value(m_container).fileExtension);
    m_sink.setUrl(sink);

    DP0("S60AudioCaptureSession::prepareSinkL ---");
}

void S60AudioCaptureSession::record()
{
    DP0("S60AudioCaptureSession::record +++");

    if (!m_recorderUtility)
        return;

    if (m_captureState == EInitialized || m_captureState == ERecordComplete) {
        prepareSinkL();
        QString filename = m_sink.toString();
        TPtrC16 sink(reinterpret_cast<const TUint16*>(filename.utf16()));
        TUid controllerUid(TUid::Uid(m_controllerIdMap.value(m_container).controllerUid));
        TUid formatUid(TUid::Uid(m_controllerIdMap.value(m_container).destinationFormatUid));

        TRAPD(err,m_recorderUtility->OpenFileL(sink));
        setError(err);
    }else if (m_captureState == EPaused) {
        m_recorderUtility->SetPosition(m_pausedPosition);
        TRAPD(error, m_recorderUtility->RecordL());
        setError(error);
        m_captureState = ERecording;
        emit stateChanged(m_captureState);
    }

    DP0("S60AudioCaptureSession::record ---");
}

void S60AudioCaptureSession::mute(bool muted)
{
    DP0("S60AudioCaptureSession::mute +++");

    if (!m_recorderUtility)
        return;

    if (muted)
        m_recorderUtility->SetGain(0);
    else
        m_recorderUtility->SetGain(m_recorderUtility->MaxGain());

    m_isMuted = muted;

    DP0("S60AudioCaptureSession::mute ---");
}

bool S60AudioCaptureSession::muted()
{
    DP0("S60AudioCaptureSession::muted");

    return m_isMuted;
}

void S60AudioCaptureSession::setDefaultSettings()
{
    DP0("S60AudioCaptureSession::setDefaultSettings +++");

    // Setting AMR to default format if supported
    if (m_controllerIdMap.count() > 0) {
        if ( m_controllerIdMap.contains("audio/amr"))
            m_container = QString("audio/amr");
        else
            m_container = m_controllerIdMap.keys()[0];
    }
    if (m_audioCodeclist.keys().count() > 0) {
        if (m_audioCodeclist.keys().contains("AMR")) {
            m_format.setSampleSize(8);
            m_format.setChannels(1);
            m_format.setFrequency(8000);
            m_format.setSampleType(QAudioFormat::SignedInt);
            m_format.setCodec("AMR");
        }else
            m_format.setCodec(m_audioCodeclist.keys()[0]);
    }

    DP0("S60AudioCaptureSession::setDefaultSettings ---");
}

void S60AudioCaptureSession::pause()
{
    DP0("S60AudioCaptureSession::pause +++");

    if (!m_recorderUtility)
        return;

    m_pausedPosition = m_recorderUtility->Position();
    m_recorderUtility->Stop();
    m_captureState = EPaused;
    emit stateChanged(m_captureState);

    DP0("S60AudioCaptureSession::pause ---");
}

void S60AudioCaptureSession::stop()
{
    DP0("S60AudioCaptureSession::stop +++");

    if (!m_recorderUtility)
        return;

    m_recorderUtility->Stop();

#ifdef AUDIOINPUT_ROUTING
    //delete audio input instance before closing the utility.
    if (m_audioInput)
     {
        delete m_audioInput;
        m_audioInput = NULL;
     }
#endif //AUDIOINPUT_ROUTING

    m_recorderUtility->Close();
    m_captureState = ERecordComplete;
    emit stateChanged(m_captureState);
}

#ifdef AUDIOINPUT_ROUTING

void S60AudioCaptureSession::initAudioInputs()
{
   DP0(" S60AudioCaptureSession::initAudioInputs +++");

    m_audioInputs[S60AudioCaptureSession::microPhone] = QString("Microphone associated with the currently active speaker.");
    m_audioInputs[S60AudioCaptureSession::voiceCall] = QString("Audio stream associated with the current phone call.");
    m_audioInputs[S60AudioCaptureSession::fmRadio] = QString("Audio of the currently tuned FM radio station.");

   DP0(" S60AudioCaptureSession::initAudioInputs ---");
}

#endif //AUDIOINPUT_ROUTING

void S60AudioCaptureSession::setActiveEndpoint(const QString& audioEndpoint)
{
  DP0(" S60AudioCaptureSession::setActiveEndpoint +++");

    if (!m_audioInputs.keys().contains(audioEndpoint))
        return;

    if (activeEndpoint().compare(audioEndpoint) != 0) {
        m_audioEndpoint = audioEndpoint;
#ifdef AUDIOINPUT_ROUTING
        m_setActiveEndPoint = TRUE;
#endif
       }

  DP0(" S60AudioCaptureSession::setActiveEndpoint ---");
}

QList<QString> S60AudioCaptureSession::availableEndpoints() const
{
    DP0(" S60AudioCaptureSession::availableEndpoints");

    return m_audioInputs.keys();
}

QString S60AudioCaptureSession::endpointDescription(const QString& name) const
{
  DP0(" S60AudioCaptureSession::endpointDescription +++");

    if (m_audioInputs.keys().contains(name))
        return m_audioInputs.value(name);
    return QString();
}

QString S60AudioCaptureSession::activeEndpoint() const
{
   DP0(" S60AudioCaptureSession::activeEndpoint");

    QString inputSourceName = NULL;
#ifdef AUDIOINPUT_ROUTING
    if (m_audioInput) {
        CAudioInput::TAudioInputArray input = m_audioInput->AudioInput();
        inputSourceName = qStringFromTAudioInputPreference(input[0]);
    }
#endif //AUDIOINPUT_ROUTING
    return inputSourceName;
}

QString S60AudioCaptureSession::defaultEndpoint() const
{
   DP0(" S60AudioCaptureSession::defaultEndpoint");

#ifdef AUDIOINPUT_ROUTING
    return QString(S60AudioCaptureSession::microPhone);
#else
    return NULL;
#endif
}

#ifdef AUDIOINPUT_ROUTING

void S60AudioCaptureSession::doSetAudioInputL(const QString& name)
{
   DP0(" S60AudioCaptureSession::doSetAudioInputL +++");
   DP1(" S60AudioCaptureSession::doSetAudioInputL:", name);
    TInt err(KErrNone);

    if (!m_recorderUtility)
        return;

    CAudioInput::TAudioInputPreference input = CAudioInput::EDefaultMic;

    if (name.compare(S60AudioCaptureSession::voiceCall) == 0)
        input = CAudioInput::EVoiceCall;
//    commented because they are not supported on 9.2
    else if (name.compare(S60AudioCaptureSession::fmRadio) == 0)
        input = CAudioInput::EFMRadio;
    else // S60AudioCaptureSession::microPhone
        input = CAudioInput::EDefaultMic;

        RArray<CAudioInput::TAudioInputPreference> inputArray;
        inputArray.Append(input);

        if (m_audioInput){
            TRAP(err,m_audioInput->SetAudioInputL(inputArray.Array()));

            if (err == KErrNone) {
                emit activeEndpointChanged(name);
            }
            else{
                setError(err);
            }
        }
        inputArray.Close();

   DP0(" S60AudioCaptureSession::doSetAudioInputL ---");
}


QString S60AudioCaptureSession::qStringFromTAudioInputPreference(CAudioInput::TAudioInputPreference input) const
{
   DP0(" S60AudioCaptureSession::qStringFromTAudioInputPreference");

    if (input == CAudioInput::EVoiceCall)
        return S60AudioCaptureSession::voiceCall;
    else if (input == CAudioInput::EFMRadio)
        return S60AudioCaptureSession::fmRadio;
    else
        return S60AudioCaptureSession::microPhone; // CAudioInput::EDefaultMic
}
#endif //AUDIOINPUT_ROUTING


void S60AudioCaptureSession::MoscoStateChangeEvent(CBase* aObject,
        TInt aPreviousState, TInt aCurrentState, TInt aErrorCode)
{
    DP0("S60AudioCaptureSession::MoscoStateChangeEvent +++");

    if (aErrorCode==KErrNone) {
	    TRAPD(err, MoscoStateChangeEventL(aObject, aPreviousState, aCurrentState, NULL));
	    setError(err);
	}
    else {
        setError(aErrorCode);
    }
  DP1("S60AudioCaptureSession::MoscoStateChangeEvent, aErrorCode:", aErrorCode);
  DP0("S60AudioCaptureSession::MoscoStateChangeEvent ---");
}

void S60AudioCaptureSession::MoscoStateChangeEventL(CBase* aObject,
        TInt aPreviousState, TInt aCurrentState, TInt aErrorCode)
{
    DP0("S60AudioCaptureSession::MoscoStateChangeEventL +++");

    DP5("S60AudioCaptureSession::MoscoStateChangeEventL - aPreviousState:", aPreviousState,
            "aCurrentState:", aCurrentState, "aErrorCode:", aErrorCode);
	if (aObject != m_recorderUtility)
	    return;

		switch(aCurrentState) {
        case CMdaAudioClipUtility::EOpen: {
            if(aPreviousState == CMdaAudioClipUtility::ENotReady) {
                applyAudioSettingsL();
                m_recorderUtility->SetGain(m_recorderUtility->MaxGain());
                TRAPD(err, m_recorderUtility->RecordL());
                setError(err);
                m_captureState = EOpenCompelete;
                emit stateChanged(m_captureState);
            }
            break;
        }
        case CMdaAudioClipUtility::ENotReady: {
            m_captureState = EInitialized;
            emit stateChanged(m_captureState);
            break;
        }
        case CMdaAudioClipUtility::ERecording: {
            m_captureState = ERecording;
            emit stateChanged(m_captureState);
            break;
        }
        default: {
            break;
        }
		}

  DP0("S60AudioCaptureSession::MoscoStateChangeEventL ---");
}

void S60AudioCaptureSession::updateAudioContainersL()
{
    DP0("S60AudioCaptureSession::updateAudioContainersL +++");

    CMMFControllerPluginSelectionParameters* pluginParameters =
    	CMMFControllerPluginSelectionParameters::NewLC();
	CMMFFormatSelectionParameters* formatParameters =
		CMMFFormatSelectionParameters::NewLC();

	pluginParameters->SetRequiredRecordFormatSupportL(*formatParameters);

	RArray<TUid> ids;
	CleanupClosePushL(ids);
	User::LeaveIfError(ids.Append(KUidMediaTypeAudio));

	pluginParameters->SetMediaIdsL(ids,
		CMMFPluginSelectionParameters::EAllowOnlySuppliedMediaIds);

	RMMFControllerImplInfoArray controllers;
	CleanupResetAndDestroyPushL(controllers);

	//Get all audio record controllers/formats that are supported
	pluginParameters->ListImplementationsL(controllers);

	for (TInt index=0; index<controllers.Count(); index++) {
		const RMMFFormatImplInfoArray& recordFormats =
			controllers[index]->RecordFormats();
		for (TInt j=0; j<recordFormats.Count(); j++) {
			const CDesC8Array& mimeTypes = recordFormats[j]->SupportedMimeTypes();
			const CDesC8Array& fileExtensions = recordFormats[j]->SupportedFileExtensions();
			TInt mimeCount = mimeTypes.Count();
			TInt fileExtCount = fileExtensions.Count();

			if (mimeCount > 0 && fileExtCount > 0) {
                TPtrC8 extension = fileExtensions[0];
                TPtrC8 mimeType = mimeTypes[0];
                QString type = QString::fromUtf8((char *)mimeType.Ptr(), mimeType.Length());

            if (type != "audio/basic") {
                    ControllerData data;
                    data.controllerUid = controllers[index]->Uid().iUid;
                    data.destinationFormatUid = recordFormats[j]->Uid().iUid;
                    data.destinationFormatDescription = QString::fromUtf16(
                            recordFormats[j]->DisplayName().Ptr(),
                            recordFormats[j]->DisplayName().Length());
                    data.fileExtension = QString::fromUtf8((char *)extension.Ptr(), extension.Length());
                    m_controllerIdMap[type] = data;
				}
			}
		}
	}
	CleanupStack::PopAndDestroy(4);//controllers, ids, formatParameters, pluginParameters

  DP0("S60AudioCaptureSession::updateAudioContainersL ---");
}

void S60AudioCaptureSession::retrieveSupportedAudioSampleRatesL()
{
    DP0("S60AudioCaptureSession::retrieveSupportedAudioSampleRatesL +++");

    if (!m_recorderUtility) {
        DP0("No RecorderUtility");
        return;
    }

    m_supportedSampleRates.clear();

    RArray<TUint> supportedSampleRates;
    CleanupClosePushL(supportedSampleRates);
    m_recorderUtility->GetSupportedSampleRatesL(supportedSampleRates);
    for (TInt j = 0; j < supportedSampleRates.Count(); j++ )
        m_supportedSampleRates.append(supportedSampleRates[j]);

    CleanupStack::PopAndDestroy(&supportedSampleRates);

    DP0("S60AudioCaptureSession::retrieveSupportedAudioSampleRatesL ---");
}

QList<int> S60AudioCaptureSession::supportedAudioSampleRates(const QAudioEncoderSettings &settings) const
{
    DP0("S60AudioCaptureSession::supportedAudioSampleRates +++");

    QList<int> supportedSampleRates;

    if (!settings.codec().isEmpty()) {
        if (settings.codec() == "AMR")
            supportedSampleRates.append(8000);
        else
            supportedSampleRates = m_supportedSampleRates;
    }else
        supportedSampleRates = m_supportedSampleRates;

    DP0("S60AudioCaptureSession::supportedAudioSampleRates ---");

    return supportedSampleRates;
}

void S60AudioCaptureSession::populateAudioCodecsDataL()
{
    DP0("S60AudioCaptureSession::populateAudioCodecsDataL +++");

    if (!m_recorderUtility) {
        DP0("No RecorderUtility");

        return;
    }

    if (m_controllerIdMap.contains("audio/amr")) {
        CodecData data;
        data.codecDescription = QString("GSM AMR Codec");
        m_audioCodeclist[QString("AMR")]=data;
    }
    if (m_controllerIdMap.contains("audio/basic")) {
        CodecData data;
        data.fourCC = KMMFFourCCCodeALAW;
        data.codecDescription = QString("Sun/Next ""Au"" audio codec");
        m_audioCodeclist[QString("AULAW")]=data;
    }
    if (m_controllerIdMap.contains("audio/wav")) {
        CodecData data;
        data.fourCC = KMMFFourCCCodePCM16;
        data.codecDescription = QString("Pulse code modulation");
        m_audioCodeclist[QString("PCM")]=data;
    }
    if (m_controllerIdMap.contains("audio/mp4")) {
        CodecData data;
        data.fourCC = KMMFFourCCCodeAAC;
        data.codecDescription = QString("Advanced Audio Codec");
        m_audioCodeclist[QString("AAC")]=data;
    }

    // default samplerates
    m_supportedSampleRates << 96000 << 88200 << 64000 << 48000 << 44100 << 32000 << 24000 << 22050 << 16000 << 12000 << 11025 << 8000;

    DP0("S60AudioCaptureSession::populateAudioCodecsDataL ---");
}

void S60AudioCaptureSession::applyAudioSettingsL()
{
   DP0("S60AudioCaptureSession::applyAudioSettingsL +++");

    if (!m_recorderUtility)
        return;

#ifdef AUDIOINPUT_ROUTING
    //CAudioInput needs to be re-initialized every time recording starts
    if (m_audioInput) {
        delete m_audioInput;
        m_audioInput = NULL;
    }

    if (m_setActiveEndPoint) {
        m_audioInput = CAudioInput::NewL(*m_recorderUtility);
        doSetAudioInputL(m_audioEndpoint);
    }
#endif //AUDIOINPUT_ROUTING

    if (m_format.codec() == "AMR")
        return;

    TFourCC fourCC = m_audioCodeclist.value(m_format.codec()).fourCC;

    if (m_format.codec() == "PCM")
        fourCC = determinePCMFormat();

    RArray<TFourCC> supportedDataTypes;
    CleanupClosePushL(supportedDataTypes);
    TRAPD(err,m_recorderUtility->GetSupportedDestinationDataTypesL(supportedDataTypes));
    TInt num = supportedDataTypes.Count();
    if (num > 0 ) {
        supportedDataTypes.SortUnsigned();
        int index = supportedDataTypes.Find(fourCC.FourCC());
         if (index != KErrNotFound) {
            TRAPD(err,m_recorderUtility->SetDestinationDataTypeL(supportedDataTypes[index]));
        }
    }

    supportedDataTypes.Reset();
    CleanupStack::PopAndDestroy(&supportedDataTypes);

    if (m_recorderUtility->DestinationSampleRateL() != m_format.frequency()) {

        RArray<TUint> supportedSampleRates;
        CleanupClosePushL(supportedSampleRates);
        m_recorderUtility->GetSupportedSampleRatesL(supportedSampleRates);
    for (TInt i = 0; i < supportedSampleRates.Count(); i++ ) {
        TUint supportedSampleRate = supportedSampleRates[i];
        if (supportedSampleRate == m_format.frequency()) {
            m_recorderUtility->SetDestinationSampleRateL(m_format.frequency());
            break;
        }
    }
        supportedSampleRates.Reset();
        CleanupStack::PopAndDestroy(&supportedSampleRates);
    }

    /* If requested channel setting is different than current one */
    if (m_recorderUtility->DestinationNumberOfChannelsL() != m_format.channels()) {
        RArray<TUint> supportedChannels;
        CleanupClosePushL(supportedChannels);
        m_recorderUtility->GetSupportedNumberOfChannelsL(supportedChannels);
        for (TInt l = 0; l < supportedChannels.Count(); l++ ) {
            if (supportedChannels[l] == m_format.channels()) {
                m_recorderUtility->SetDestinationNumberOfChannelsL(m_format.channels());
                break;
            }
        }
        supportedChannels.Reset();
        CleanupStack::PopAndDestroy(&supportedChannels);
    }

    if (!(m_format.codec().compare("pcm",Qt::CaseInsensitive) == 0)) {
        if (m_recorderUtility->DestinationBitRateL() != m_audioEncoderSettings.bitRate()) {
            RArray<TUint> supportedBitRates;
            CleanupClosePushL(supportedBitRates);
            m_recorderUtility->GetSupportedBitRatesL(supportedBitRates);
            for (TInt l = 0; l < supportedBitRates.Count(); l++ ) {
                if (supportedBitRates[l] == m_audioEncoderSettings.bitRate()) {
                    m_recorderUtility->SetDestinationBitRateL(m_audioEncoderSettings.bitRate());
                    break;
                }
            }
            supportedBitRates.Reset();
            CleanupStack::PopAndDestroy(&supportedBitRates);
        }
    }

    DP0("S60AudioCaptureSession::applyAudioSettingsL ---");
}

TFourCC S60AudioCaptureSession::determinePCMFormat()
{
    DP0("S60AudioCaptureSession::determinePCMFormat +++");

    TFourCC fourCC;

    if (m_format.sampleSize() == 8) {
        // 8 bit
        switch (m_format.sampleType()) {
        case QAudioFormat::SignedInt: {
            fourCC.Set(KMMFFourCCCodePCM8);
            break;
        }
        case QAudioFormat::UnSignedInt: {
            fourCC.Set(KMMFFourCCCodePCMU8);
            break;
        }
        case QAudioFormat::Float:
        case QAudioFormat::Unknown:
        default: {
            fourCC.Set(KMMFFourCCCodePCM8);
            break;
        }
        }
    } else if (m_format.sampleSize() == 16) {
        // 16 bit
        switch (m_format.sampleType()) {
        case QAudioFormat::SignedInt: {
            fourCC.Set(m_format.byteOrder()==QAudioFormat::BigEndian?
                KMMFFourCCCodePCM16B:KMMFFourCCCodePCM16);
            break;
        }
        case QAudioFormat::UnSignedInt: {
            fourCC.Set(m_format.byteOrder()==QAudioFormat::BigEndian?
                KMMFFourCCCodePCMU16B:KMMFFourCCCodePCMU16);
            break;
        }
        default: {
            fourCC.Set(KMMFFourCCCodePCM16);
            break;
        }
        }
    }

    DP0("S60AudioCaptureSession::determinePCMFormat ---");

    return fourCC;
}
