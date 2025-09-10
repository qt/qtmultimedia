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

namespace {
SLAndroidDataFormat_PCM_EX getDefaultFormat()
{
    SLAndroidDataFormat_PCM_EX ret;
    ret.formatType = SL_DATAFORMAT_PCM;
    ret.numChannels = 1;
    ret.sampleRate = SL_SAMPLINGRATE_44_1;
    ret.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_32;
    ret.containerSize = SL_PCMSAMPLEFORMAT_FIXED_32;
    ret.channelMask = SL_ANDROID_MAKE_INDEXED_CHANNEL_MASK(SL_SPEAKER_FRONT_CENTER);
    ret.endianness = SL_BYTEORDER_LITTLEENDIAN;
    ret.representation = SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT;

    return ret;
}

SLuint32 getChannelMask(unsigned channelCount)
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
    case 7: return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT | SL_SPEAKER_FRONT_CENTER
                | SL_SPEAKER_BACK_LEFT | SL_SPEAKER_BACK_RIGHT | SL_SPEAKER_LOW_FREQUENCY
                | SL_SPEAKER_BACK_CENTER;
    case 8: return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT | SL_SPEAKER_FRONT_CENTER
                | SL_SPEAKER_LOW_FREQUENCY | SL_SPEAKER_BACK_LEFT | SL_SPEAKER_BACK_RIGHT
                | SL_SPEAKER_SIDE_LEFT | SL_SPEAKER_SIDE_RIGHT;
    case 9: return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT | SL_SPEAKER_FRONT_CENTER
                | SL_SPEAKER_LOW_FREQUENCY | SL_SPEAKER_BACK_LEFT | SL_SPEAKER_BACK_RIGHT
                | SL_SPEAKER_SIDE_LEFT | SL_SPEAKER_SIDE_RIGHT | SL_SPEAKER_TOP_FRONT_CENTER;
    case 10: return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT | SL_SPEAKER_FRONT_CENTER
                | SL_SPEAKER_LOW_FREQUENCY | SL_SPEAKER_BACK_LEFT | SL_SPEAKER_BACK_RIGHT
                | SL_SPEAKER_SIDE_LEFT | SL_SPEAKER_SIDE_RIGHT | SL_SPEAKER_TOP_FRONT_LEFT
                | SL_SPEAKER_TOP_FRONT_RIGHT;
    case 11: return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT | SL_SPEAKER_FRONT_CENTER
                | SL_SPEAKER_LOW_FREQUENCY | SL_SPEAKER_BACK_LEFT | SL_SPEAKER_BACK_RIGHT
                | SL_SPEAKER_SIDE_LEFT | SL_SPEAKER_SIDE_RIGHT | SL_SPEAKER_TOP_FRONT_LEFT
                | SL_SPEAKER_TOP_FRONT_RIGHT | SL_SPEAKER_TOP_BACK_CENTER;
    case 12: return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT | SL_SPEAKER_FRONT_CENTER
                | SL_SPEAKER_LOW_FREQUENCY | SL_SPEAKER_BACK_LEFT | SL_SPEAKER_BACK_RIGHT
                | SL_SPEAKER_SIDE_LEFT | SL_SPEAKER_SIDE_RIGHT | SL_SPEAKER_TOP_FRONT_LEFT
                | SL_SPEAKER_TOP_FRONT_RIGHT | SL_SPEAKER_TOP_BACK_LEFT
                | SL_SPEAKER_TOP_BACK_RIGHT;
    default: return 0; // Default to 0 for an unsupported or unknown number of channels
    }
}

SLuint32 getRepresentation(QAudioFormat::SampleFormat format) {
    switch (format) {
    case QAudioFormat::SampleFormat::UInt8: return SL_ANDROID_PCM_REPRESENTATION_UNSIGNED_INT;
    case QAudioFormat::SampleFormat::Int16:
    case QAudioFormat::SampleFormat::Int32: return SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT;
    case QAudioFormat::SampleFormat::Float: return SL_ANDROID_PCM_REPRESENTATION_FLOAT;
    case QAudioFormat::SampleFormat::NSampleFormats:
    case QAudioFormat::SampleFormat::Unknown:
    default: return SL_ANDROID_PCM_REPRESENTATION_INVALID;
    }
}

bool hasRecordPermission()
{
    return qApp->checkPermission(QMicrophonePermission{}) == Qt::PermissionStatus::Granted;
}
} // namespace

QOpenSLESEngine::QOpenSLESEngine()
    : m_engineObject(0)
    , m_engine(0)
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
    format_pcm.representation = getRepresentation(format.sampleFormat());

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
              auto devElement = env->GetObjectArrayElement(devsArray, i);
              QString val = QJniObject(devElement).toString();
              env->DeleteLocalRef(devElement);
              int pos = val.indexOf(QStringLiteral(":"));
              devices << QAudioDevicePrivate::createQAudioDevice(
                      std::make_unique<QOpenSLESDeviceInfo>(val.left(pos).toUtf8(),
                                                            val.mid(pos + 1), mode, i == 0));
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

QList<int> QOpenSLESEngine::supportedChannelCounts(QAudioDevice::Mode mode)
{
    return (mode == QAudioDevice::Input) ?
        m_supportedInput.ensure([this] () {
          return getSupportedInputConfigs();
        }).m_channelCounts
      : m_supportedOutput.ensure([this] () {
          return getSupportedOutputConfigs();
        }).m_channelCounts;
}

QList<int> QOpenSLESEngine::supportedSampleRates(QAudioDevice::Mode mode)
{
    return (mode == QAudioDevice::Input) ?
        m_supportedInput.ensure([this] () {
          return getSupportedInputConfigs();
        }).m_sampleRates
      : m_supportedOutput.ensure([this] () {
          return getSupportedOutputConfigs();
        }).m_sampleRates;
}

QList<QAudioFormat::SampleFormat> QOpenSLESEngine::supportedSampleFormats(
        QAudioDevice::Mode mode)
{
    return (mode == QAudioDevice::Input) ?
        m_supportedInput.ensure([this] () {
          return getSupportedInputConfigs();
        }).m_sampleFormats
      : m_supportedOutput.ensure([this] () {
          return getSupportedOutputConfigs();
        }).m_sampleFormats;
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

QOpenSLESEngine::AudioConfig QOpenSLESEngine::getSupportedInputConfigs()
{
    QOpenSLESEngine::AudioConfig ret;
    auto defaultFormat = getDefaultFormat();
    constexpr SLuint32 rates[13] = { SL_SAMPLINGRATE_8,
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
    for (const auto r : rates) {
        auto format = defaultFormat;
        format.sampleRate = r;
        if (inputFormatIsSupported(format)) {
            ret.m_sampleRates.append(r / 1000);
            if (ret.m_channelCounts.empty())
                // Add one supported channel if any of sample rates supported
                ret.m_channelCounts.append(1);
            continue;
        }

        // If sample rates were not supported, lets check again with numChannels == 2
        format.numChannels = 2;
        format.channelMask = SL_ANDROID_MAKE_INDEXED_CHANNEL_MASK(SL_SPEAKER_FRONT_LEFT
                                                                  | SL_SPEAKER_FRONT_RIGHT);
        if (inputFormatIsSupported(format))
            ret.m_sampleRates.append(r / 1000);
    }
    // Test if stereo is supported
    {
        SLAndroidDataFormat_PCM_EX format = defaultFormat;
        format.numChannels = 2;
        format.channelMask = SL_ANDROID_MAKE_INDEXED_CHANNEL_MASK(SL_SPEAKER_FRONT_LEFT
                                                                  | SL_SPEAKER_FRONT_RIGHT);
        if (inputFormatIsSupported(format))
            ret.m_channelCounts.append(2);
    }

    // Test sample Formats
    ret.m_sampleFormats = getSupportedSampleFormats(QAudioDevice::Input);

    return ret;
}

QOpenSLESEngine::AudioConfig QOpenSLESEngine::getSupportedOutputConfigs()
{
    QOpenSLESEngine::AudioConfig ret;
    static const QList<int> sampleRates = {
        8000, 11025, 12000, 16000, 22050, 24000, 32000,
        44100, 48000, 64000, 88200, 96000, 192000
    };
    ret.m_sampleRates = sampleRates;
    ret.m_channelCounts = getSupportedOutputChannelCounts();
    ret.m_sampleFormats = getSupportedSampleFormats(QAudioDevice::Output);
    return ret;
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
    if (result == SL_RESULT_SUCCESS) {
        result = (*recorder)->Realize(recorder, false);
        (*recorder)->Destroy(recorder);
    }

    return result == SL_RESULT_SUCCESS;
}

QList<QAudioFormat::SampleFormat> QOpenSLESEngine::getSupportedSampleFormats(
        SLAndroidDataFormat_PCM_EX format, QAudioDevice::Mode mode)
{
    QList<QAudioFormat::SampleFormat> ret;
    const auto sampleFormats = qAllSupportedSampleFormats();

    auto getSize = [](QAudioFormat::SampleFormat f) {
        switch (f) {
        case QAudioFormat::UInt8: return SL_PCMSAMPLEFORMAT_FIXED_8;
        case QAudioFormat::Int16: return SL_PCMSAMPLEFORMAT_FIXED_16;
        case QAudioFormat::Int32: return SL_PCMSAMPLEFORMAT_FIXED_32;
        case QAudioFormat::Float: return SL_PCMSAMPLEFORMAT_FIXED_32;
        default: Q_UNREACHABLE_RETURN(SL_PCMSAMPLEFORMAT_FIXED_8);
        }
    };

    for (const auto sampleFormat : sampleFormats) {
        format.representation = getRepresentation(sampleFormat);
        format.bitsPerSample = getSize(sampleFormat);
        format.containerSize = format.bitsPerSample;
        if (mode == QAudioDevice::Input) {
            if (inputFormatIsSupported(format))
                ret.append(sampleFormat);
        } else {
            if (outputFormatIsSupported(format))
                ret.append(sampleFormat);
        }
    }

    return ret;
}

QList<QAudioFormat::SampleFormat> QOpenSLESEngine::getSupportedSampleFormats(
        QAudioDevice::Mode mode)
{
    QList<QAudioFormat::SampleFormat> ret;
    auto format = getDefaultFormat();
    ret = getSupportedSampleFormats(format, mode);

    if (ret.empty()) {
        // Try once again with channel == 2. On some devices like x86 emulator with API level 28,
        // mono (1-channel) audio is not supported, while stereo (2-channel) audio is supported
        format.numChannels = 2;
        format.channelMask = SL_ANDROID_MAKE_INDEXED_CHANNEL_MASK(SL_SPEAKER_FRONT_LEFT
                                                                  | SL_SPEAKER_FRONT_RIGHT);
        ret = getSupportedSampleFormats(format, mode);
    }

    return ret;
}

QList<int> QOpenSLESEngine::getSupportedOutputChannelCounts()
{
    QList<int> ret;
    constexpr int possibleChannelCounts[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
    auto format = getDefaultFormat();

    for (int channels : possibleChannelCounts) {
        format.numChannels = channels;
        format.channelMask = getChannelMask(channels);
        if (outputFormatIsSupported(format))
            ret.append(channels);
    }

    return ret;
}

bool QOpenSLESEngine::outputFormatIsSupported(const SLAndroidDataFormat_PCM_EX& format) const
{
    SLresult result;
    SLObjectItf player = nullptr;
    SLObjectItf outputMixObject = nullptr;

    result = (*m_engine)->CreateOutputMix(m_engine, &outputMixObject, 0, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS)
        return false;

    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        (*outputMixObject)->Destroy(outputMixObject);
        return false;
    }

    SLDataLocator_AndroidSimpleBufferQueue loc_bq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1 };
    SLDataSource audioSrc = { &loc_bq, (void*)&format };

    SLDataLocator_OutputMix loc_outmix = { SL_DATALOCATOR_OUTPUTMIX, outputMixObject };
    SLDataSink audioSnk = { &loc_outmix, nullptr };

    result = (*m_engine)->CreateAudioPlayer(m_engine, &player, &audioSrc, &audioSnk, 0, nullptr, nullptr);

    if (result == SL_RESULT_SUCCESS) {
        result = (*player)->Realize(player, SL_BOOLEAN_FALSE);
        (*player)->Destroy(player);
    }

    (*outputMixObject)->Destroy(outputMixObject);

    return result == SL_RESULT_SUCCESS;
}
