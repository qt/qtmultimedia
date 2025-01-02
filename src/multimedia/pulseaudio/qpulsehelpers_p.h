// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPULSEHELPER_H
#define QPULSEHELPER_H

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

#include "qaudiodevice.h"
#include <qaudioformat.h>
#include <pulse/pulseaudio.h>
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcPulseAudioOut)

struct PAOperationDeleter
{
    void operator()(pa_operation *op) const { pa_operation_unref(op); }
};

using PAOperationUPtr = std::unique_ptr<pa_operation, PAOperationDeleter>;

namespace QPulseAudioInternal
{
pa_sample_spec audioFormatToSampleSpec(const QAudioFormat &format);
QAudioFormat sampleSpecToAudioFormat(const pa_sample_spec &spec);
pa_channel_map channelMapForAudioFormat(const QAudioFormat &format);
QAudioFormat::ChannelConfig channelConfigFromMap(const pa_channel_map &map);

static inline QString stateToQString(pa_stream_state_t state)
{
    using namespace Qt::StringLiterals;
    switch (state)
    {
    case PA_STREAM_UNCONNECTED: return "Unconnected"_L1;
    case PA_STREAM_CREATING:    return "Creating"_L1;
    case PA_STREAM_READY:       return "Ready"_L1;
    case PA_STREAM_FAILED:      return "Failed"_L1;
    case PA_STREAM_TERMINATED:  return "Terminated"_L1;
    }

    return u"Unknown state: %0"_s.arg(int(state));
}

static inline QString sampleFormatToQString(pa_sample_format format)
{
    using namespace Qt::StringLiterals;
    switch (format)
    {
    case PA_SAMPLE_U8:          return "Unsigned 8 Bit PCM."_L1;
    case PA_SAMPLE_ALAW:        return "8 Bit a-Law "_L1;
    case PA_SAMPLE_ULAW:        return "8 Bit mu-Law"_L1;
    case PA_SAMPLE_S16LE:       return "Signed 16 Bit PCM, little endian (PC)."_L1;
    case PA_SAMPLE_S16BE:       return "Signed 16 Bit PCM, big endian."_L1;
    case PA_SAMPLE_FLOAT32LE:   return "32 Bit IEEE floating point, little endian (PC), range -1.0 to 1.0"_L1;
    case PA_SAMPLE_FLOAT32BE:   return "32 Bit IEEE floating point, big endian, range -1.0 to 1.0"_L1;
    case PA_SAMPLE_S32LE:       return "Signed 32 Bit PCM, little endian (PC)."_L1;
    case PA_SAMPLE_S32BE:       return "Signed 32 Bit PCM, big endian."_L1;
    case PA_SAMPLE_S24LE:       return "Signed 24 Bit PCM packed, little endian (PC)."_L1;
    case PA_SAMPLE_S24BE:       return "Signed 24 Bit PCM packed, big endian."_L1;
    case PA_SAMPLE_S24_32LE:    return "Signed 24 Bit PCM in LSB of 32 Bit words, little endian (PC)."_L1;
    case PA_SAMPLE_S24_32BE:    return "Signed 24 Bit PCM in LSB of 32 Bit words, big endian."_L1;
    case PA_SAMPLE_MAX:         return "Upper limit of valid sample types."_L1;
    case PA_SAMPLE_INVALID:     return "Invalid sample format"_L1;
    }

    return u"Invalid value: %0"_s.arg(int(format));
}

static inline QString stateToQString(pa_context_state_t state)
{
    using namespace Qt::StringLiterals;
    switch (state)
    {
    case PA_CONTEXT_UNCONNECTED:  return "Unconnected"_L1;
    case PA_CONTEXT_CONNECTING:   return "Connecting"_L1;
    case PA_CONTEXT_AUTHORIZING:  return "Authorizing"_L1;
    case PA_CONTEXT_SETTING_NAME: return "Setting Name"_L1;
    case PA_CONTEXT_READY:        return "Ready"_L1;
    case PA_CONTEXT_FAILED:       return "Failed"_L1;
    case PA_CONTEXT_TERMINATED:   return "Terminated"_L1;
    }

    return u"Unknown state: %0"_s.arg(int(state));
}
}

QT_END_NAMESPACE

#endif
