// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qopenslesengine_p.h"

#include "qandroidaudiosource_p.h"
#include "qandroidaudiodevice_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qpermissions.h>
#include <QtCore/private/qandroidextras_p.h>
#include <qdebug.h>

#define MINIMUM_PERIOD_TIME_MS 5
#define DEFAULT_PERIOD_TIME_MS 50

#define CheckError(message) if (result != SL_RESULT_SUCCESS) { qWarning(message); return; }

#define SL_ANDROID_PCM_REPRESENTATION_INVALID 0

Q_GLOBAL_STATIC(QOpenSLESEngine, openslesEngine);

QOpenSLESEngine::QOpenSLESEngine()
    : m_engineObject(0)
    , m_engine(0)
    , m_checkedInputFormats(false)
{
    SLresult result;

    result = slCreateEngine(&m_engineObject, 0, 0, 0, 0, 0);
    CheckError("Failed to create engine");

    result = (*m_engineObject)->Realize(m_engineObject, SL_BOOLEAN_FALSE);
    CheckError("Failed to realize engine");

    result = (*m_engineObject)->GetInterface(m_engineObject, SL_IID_ENGINE, &m_engine);
    CheckError("Failed to get engine interface");
}

QOpenSLESEngine::~QOpenSLESEngine()
{
    if (m_engineObject)
        (*m_engineObject)->Destroy(m_engineObject);
}

QOpenSLESEngine *QOpenSLESEngine::instance()
{
    return openslesEngine();
}

static SLuint32 getChannelMask(unsigned channelCount)
{
    switch (channelCount) {
        case 1: return SL_SPEAKER_FRONT_CENTER;
        case 2: return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
        case 3: return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT | SL_SPEAKER_FRONT_CENTER;
        case 4: return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT
                      | SL_SPEAKER_BACK_LEFT | SL_SPEAKER_BACK_RIGHT;
        case 5: return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT | SL_SPEAKER_BACK_LEFT
                      | SL_SPEAKER_BACK_RIGHT | SL_SPEAKER_FRONT_CENTER;
        case 6: return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT | SL_SPEAKER_BACK_LEFT
                      | SL_SPEAKER_BACK_RIGHT | SL_SPEAKER_FRONT_CENTER | SL_SPEAKER_LOW_FREQUENCY;
        default: return 0; // Default to 0 for an unsupported or unknown number of channels
    }
}

SLAndroidDataFormat_PCM_EX QOpenSLESEngine::audioFormatToSLFormatPCM(const QAudioFormat &format)
{
    SLAndroidDataFormat_PCM_EX format_pcm;
    format_pcm.formatType = SL_ANDROID_DATAFORMAT_PCM_EX;
    format_pcm.numChannels = format.channelCount();
    format_pcm.sampleRate = format.sampleRate() * 1000;
    format_pcm.bitsPerSample = format.bytesPerSample() * 8;
    format_pcm.containerSize = format.bytesPerSample() * 8;
    format_pcm.channelMask = getChannelMask(format_pcm.numChannels);
    format_pcm.endianness = (QSysInfo::ByteOrder == QSysInfo::LittleEndian ?
                                 SL_BYTEORDER_LITTLEENDIAN :
                                 SL_BYTEORDER_BIGENDIAN);

    switch (format.sampleFormat()) {
    case QAudioFormat::SampleFormat::UInt8:
        format_pcm.representation = SL_ANDROID_PCM_REPRESENTATION_UNSIGNED_INT;
        break;
    case QAudioFormat::SampleFormat::Int16:
    case QAudioFormat::SampleFormat::Int32:
        format_pcm.representation = SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT;
        break;
    case QAudioFormat::SampleFormat::Float:
        format_pcm.representation = SL_ANDROID_PCM_REPRESENTATION_FLOAT;
        break;
    case QAudioFormat::SampleFormat::NSampleFormats:
    case QAudioFormat::SampleFormat::Unknown:
        format_pcm.representation = SL_ANDROID_PCM_REPRESENTATION_INVALID;
        break;
    }

    return format_pcm;
}

QList<QAudioDevice> QOpenSLESEngine::availableDevices(QAudioDevice::Mode mode)
{
    QList<QAudioDevice> devices;
    QJniObject devs;
    if (mode == QAudioDevice::Input) {
        devs = QJniObject::callStaticObjectMethod(
                    "org/qtproject/qt/android/multimedia/QtAudioDeviceManager",
                    "getAudioInputDevices",
                    "()[Ljava/lang/String;");
    } else if (mode == QAudioDevice::Output) {
        devs = QJniObject::callStaticObjectMethod(
                    "org/qtproject/qt/android/multimedia/QtAudioDeviceManager",
                    "getAudioOutputDevices",
                    "()[Ljava/lang/String;");
    }
    if (devs.isValid()) {
          QJniEnvironment env;
          jobjectArray devsArray = static_cast<jobjectArray>(devs.object());
          const jint size = env->GetArrayLength(devsArray);
          for (int i = 0; i < size; ++i) {
              QString val = QJniObject(env->GetObjectArrayElement(devsArray, i)).toString();
              int pos = val.indexOf(QStringLiteral(":"));
              devices << (new QOpenSLESDeviceInfo(
                              val.left(pos).toUtf8(), val.mid(pos+1), mode))->create();
          }
    }
    return devices;
}

bool QOpenSLESEngine::setAudioOutput(const QByteArray &deviceId)
{
    return QJniObject::callStaticMethod<jboolean>(
                                    "org/qtproject/qt/android/multimedia/QtAudioDeviceManager",
                                    "setAudioOutput",
                                    deviceId.toInt());
}

static bool hasRecordPermission()
{
    return qApp->checkPermission(QMicrophonePermission{}) == Qt::PermissionStatus::Granted;
}

QList<int> QOpenSLESEngine::supportedChannelCounts(QAudioDevice::Mode mode) const
{
    if (mode == QAudioDevice::Input) {
        if (!m_checkedInputFormats)
            const_cast<QOpenSLESEngine *>(this)->checkSupportedInputFormats();
        return m_supportedInputChannelCounts;
    } else {
        return QList<int>() << 1 << 2;
    }
}

QList<int> QOpenSLESEngine::supportedSampleRates(QAudioDevice::Mode mode) const
{
    if (mode == QAudioDevice::Input) {
        if (!m_checkedInputFormats)
            const_cast<QOpenSLESEngine *>(this)->checkSupportedInputFormats();
        return m_supportedInputSampleRates;
    } else {
        return QList<int>() << 8000 << 11025 << 12000 << 16000 << 22050 << 24000
                            << 32000 << 44100 << 48000 << 64000 << 88200 << 96000 << 192000;
    }
}

int QOpenSLESEngine::getOutputValue(QOpenSLESEngine::OutputValue type, int defaultValue)
{
    static int sampleRate = 0;
    static int framesPerBuffer = 0;

    if (type == FramesPerBuffer && framesPerBuffer != 0)
        return framesPerBuffer;

    if (type == SampleRate && sampleRate != 0)
        return sampleRate;

    QJniObject ctx(QNativeInterface::QAndroidApplication::context());
    if (!ctx.isValid())
        return defaultValue;


    QJniObject audioServiceString = ctx.getStaticObjectField("android/content/Context",
                                                             "AUDIO_SERVICE",
                                                             "Ljava/lang/String;");
    QJniObject am = ctx.callObjectMethod("getSystemService",
                                         "(Ljava/lang/String;)Ljava/lang/Object;",
                                         audioServiceString.object());
    if (!am.isValid())
        return defaultValue;

    auto sampleRateField = QJniObject::getStaticObjectField("android/media/AudioManager",
                                                                  "PROPERTY_OUTPUT_SAMPLE_RATE",
                                                                  "Ljava/lang/String;");
    auto framesPerBufferField = QJniObject::getStaticObjectField(
                                                            "android/media/AudioManager",
                                                            "PROPERTY_OUTPUT_FRAMES_PER_BUFFER",
                                                            "Ljava/lang/String;");

    auto sampleRateString = am.callObjectMethod("getProperty",
                                                "(Ljava/lang/String;)Ljava/lang/String;",
                                                sampleRateField.object());
    auto framesPerBufferString = am.callObjectMethod("getProperty",
                                                     "(Ljava/lang/String;)Ljava/lang/String;",
                                                     framesPerBufferField.object());

    if (!sampleRateString.isValid() || !framesPerBufferString.isValid())
        return defaultValue;

    framesPerBuffer = framesPerBufferString.toString().toInt();
    sampleRate = sampleRateString.toString().toInt();

    if (type == FramesPerBuffer)
        return framesPerBuffer;

    if (type == SampleRate)
        return sampleRate;

    return defaultValue;
}

int QOpenSLESEngine::getDefaultBufferSize(const QAudioFormat &format)
{
    if (!format.isValid())
        return 0;

    const int channelConfig = [&format]() -> int
    {
        if (format.channelCount() == 1)
            return 4; /* MONO */
        else if (format.channelCount() == 2)
            return 12; /* STEREO */
        else if (format.channelCount() > 2)
            return 1052; /* SURROUND */
        else
            return 1; /* DEFAULT */
    }();

    const int audioFormat = [&format]() -> int
    {
        const int sdkVersion = QNativeInterface::QAndroidApplication::sdkVersion();
        if (format.sampleFormat() == QAudioFormat::Float && sdkVersion >= 21)
            return 4; /* PCM_FLOAT */
        else if (format.sampleFormat() == QAudioFormat::UInt8)
            return 3; /* PCM_8BIT */
        else if (format.sampleFormat() == QAudioFormat::Int16)
            return 2; /* PCM_16BIT*/
        else
            return 1; /* DEFAULT */
    }();

    const int sampleRate = format.sampleRate();
    const int minBufferSize = QJniObject::callStaticMethod<jint>("android/media/AudioTrack",
                                                                 "getMinBufferSize",
                                                                 "(III)I",
                                                                 sampleRate,
                                                                 channelConfig,
                                                                 audioFormat);
    return minBufferSize > 0 ? minBufferSize : format.bytesForDuration(DEFAULT_PERIOD_TIME_MS);
}

int QOpenSLESEngine::getLowLatencyBufferSize(const QAudioFormat &format)
{
    return format.bytesForFrames(QOpenSLESEngine::getOutputValue(QOpenSLESEngine::FramesPerBuffer,
                                                                 format.framesForDuration(MINIMUM_PERIOD_TIME_MS)));
}

bool QOpenSLESEngine::supportsLowLatency()
{
    static int isSupported = -1;

    if (isSupported != -1)
        return (isSupported == 1);

    QJniObject ctx(QNativeInterface::QAndroidApplication::context());
    if (!ctx.isValid())
        return false;

    QJniObject pm = ctx.callObjectMethod("getPackageManager", "()Landroid/content/pm/PackageManager;");
    if (!pm.isValid())
        return false;

    QJniObject audioFeatureField = QJniObject::getStaticObjectField(
                                                            "android/content/pm/PackageManager",
                                                            "FEATURE_AUDIO_LOW_LATENCY",
                                                            "Ljava/lang/String;");
    if (!audioFeatureField.isValid())
        return false;

    isSupported = pm.callMethod<jboolean>("hasSystemFeature",
                                          "(Ljava/lang/String;)Z",
                                          audioFeatureField.object());
    return (isSupported == 1);
}

bool QOpenSLESEngine::printDebugInfo()
{
    return qEnvironmentVariableIsSet("QT_OPENSL_INFO");
}

void QOpenSLESEngine::checkSupportedInputFormats()
{
    m_supportedInputChannelCounts = QList<int>() << 1;
    m_supportedInputSampleRates.clear();

    SLAndroidDataFormat_PCM_EX defaultFormat;
    defaultFormat.formatType = SL_DATAFORMAT_PCM;
    defaultFormat.numChannels = 1;
    defaultFormat.sampleRate = SL_SAMPLINGRATE_44_1;
    defaultFormat.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_32;
    defaultFormat.containerSize = SL_PCMSAMPLEFORMAT_FIXED_32;
    defaultFormat.channelMask = SL_ANDROID_MAKE_INDEXED_CHANNEL_MASK(SL_SPEAKER_FRONT_CENTER);
    defaultFormat.endianness = SL_BYTEORDER_LITTLEENDIAN;
    defaultFormat.representation = SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT;

    const SLuint32 rates[13] = { SL_SAMPLINGRATE_8,
                                SL_SAMPLINGRATE_11_025,
                                SL_SAMPLINGRATE_12,
                                SL_SAMPLINGRATE_16,
                                SL_SAMPLINGRATE_22_05,
                                SL_SAMPLINGRATE_24,
                                SL_SAMPLINGRATE_32,
                                SL_SAMPLINGRATE_44_1,
                                SL_SAMPLINGRATE_48,
                                SL_SAMPLINGRATE_64,
                                SL_SAMPLINGRATE_88_2,
                                SL_SAMPLINGRATE_96,
                                SL_SAMPLINGRATE_192 };


    // Test sampling rates
    for (size_t i = 0 ; i < std::size(rates); ++i) {
        SLAndroidDataFormat_PCM_EX format = defaultFormat;
        format.sampleRate = rates[i];

        if (inputFormatIsSupported(format))
            m_supportedInputSampleRates.append(rates[i] / 1000);

    }

    // Test if stereo is supported
    {
        SLAndroidDataFormat_PCM_EX format = defaultFormat;
        format.numChannels = 2;
        format.channelMask = SL_ANDROID_MAKE_INDEXED_CHANNEL_MASK(SL_SPEAKER_FRONT_LEFT
                                                                  | SL_SPEAKER_FRONT_RIGHT);
        if (inputFormatIsSupported(format))
            m_supportedInputChannelCounts.append(2);
    }

    m_checkedInputFormats = true;
}

bool QOpenSLESEngine::inputFormatIsSupported(SLAndroidDataFormat_PCM_EX format)
{
    SLresult result;
    SLObjectItf recorder = 0;
    SLDataLocator_IODevice loc_dev = { SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
                                       SL_DEFAULTDEVICEID_AUDIOINPUT, NULL };
    SLDataSource audioSrc = { &loc_dev, NULL };

    SLDataLocator_AndroidSimpleBufferQueue loc_bq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1 };
    SLDataSink audioSnk = { &loc_bq, &format };

    // only ask permission when it is about to create the audiorecorder
    if (!hasRecordPermission())
        return false;

    result = (*m_engine)->CreateAudioRecorder(m_engine, &recorder, &audioSrc, &audioSnk, 0, 0, 0);
    if (result == SL_RESULT_SUCCESS)
        result = (*recorder)->Realize(recorder, false);

    if (result == SL_RESULT_SUCCESS) {
        (*recorder)->Destroy(recorder);
        return true;
    }

    return false;
}
