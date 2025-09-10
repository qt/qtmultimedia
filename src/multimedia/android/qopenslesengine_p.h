// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENSLESENGINE_H
#define QOPENSLESENGINE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qglobal.h>
#include <qaudio.h>
#include <qcachedvalue_p.h>
#include <qlist.h>
#include <qaudioformat.h>
#include <qaudiodevice.h>
#include <SLES/OpenSLES_Android.h>

QT_BEGIN_NAMESPACE

class QOpenSLESEngine
{
public:
    enum OutputValue { FramesPerBuffer, SampleRate };

    QOpenSLESEngine();
    ~QOpenSLESEngine();

    static QOpenSLESEngine *instance();

    SLEngineItf slEngine() const { return m_engine; }

    static SLAndroidDataFormat_PCM_EX audioFormatToSLFormatPCM(const QAudioFormat &format);

    static QList<QAudioDevice> availableDevices(QAudioDevice::Mode mode);
    static bool setAudioOutput(const QByteArray &deviceId);
    QList<int> supportedChannelCounts(QAudioDevice::Mode mode);
    QList<int> supportedSampleRates(QAudioDevice::Mode mode);
    QList<QAudioFormat::SampleFormat> supportedSampleFormats(QAudioDevice::Mode mode);

    static int getOutputValue(OutputValue type, int defaultValue = 0);
    static int getDefaultBufferSize(const QAudioFormat &format);
    static int getLowLatencyBufferSize(const QAudioFormat &format);
    static bool supportsLowLatency();
    static bool printDebugInfo();

private:
    struct AudioConfig {
        QList<int> m_channelCounts;
        QList<int> m_sampleRates;
        QList<QAudioFormat::SampleFormat> m_sampleFormats;
    };

    AudioConfig getSupportedInputConfigs();
    AudioConfig getSupportedOutputConfigs();

    QList<QAudioFormat::SampleFormat> getSupportedSampleFormats(SLAndroidDataFormat_PCM_EX format,
                                                                QAudioDevice::Mode mode);
    QList<QAudioFormat::SampleFormat> getSupportedSampleFormats(QAudioDevice::Mode mode);
    QList<int> getSupportedOutputChannelCounts();
    bool inputFormatIsSupported(SLAndroidDataFormat_PCM_EX format);
    bool outputFormatIsSupported(const SLAndroidDataFormat_PCM_EX& format) const;
    SLObjectItf m_engineObject;
    SLEngineItf m_engine;

    QCachedValue<AudioConfig> m_supportedInput;
    QCachedValue<AudioConfig> m_supportedOutput;

};

QT_END_NAMESPACE

#endif // QOPENSLESENGINE_H
