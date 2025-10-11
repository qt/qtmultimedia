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
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcPulseAudioOut)
Q_DECLARE_LOGGING_CATEGORY(qLcPulseAudioIn)
Q_DECLARE_LOGGING_CATEGORY(qLcPulseAudioEngine)

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

QUtf8StringView currentError(const pa_context *);
QUtf8StringView currentError(const pa_stream *);

} // namespace QPulseAudioInternal

QDebug operator<<(QDebug, pa_stream_state_t);
QDebug operator<<(QDebug, pa_sample_format);
QDebug operator<<(QDebug, pa_context_state_t);

QT_END_NAMESPACE

#endif
