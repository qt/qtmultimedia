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

#include <QtMultimedia/private/qsharedhandle_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcPulseAudioOut)
Q_DECLARE_LOGGING_CATEGORY(qLcPulseAudioIn)
Q_DECLARE_LOGGING_CATEGORY(qLcPulseAudioEngine)

namespace QPulseAudioInternal
{

template <typename TypeArg, TypeArg *(*RefFn)(TypeArg *), void (*UnrefFn)(TypeArg *)>
struct PaHandleTraits
{
    using Type = TypeArg *;
    static constexpr Type invalidValue() noexcept { return nullptr; }

    static Type ref(Type handle)
    {
        Type ret = (*RefFn)(handle);
        return ret;
    }
    static bool unref(Type handle)
    {
        (*UnrefFn)(handle);
        return true;
    }
};

using PAOperationHandleTraits = PaHandleTraits<pa_operation, pa_operation_ref, pa_operation_unref>;
using PAContextHandleTraits = PaHandleTraits<pa_context, pa_context_ref, pa_context_unref>;
using PAStreamHandleTraits = PaHandleTraits<pa_stream, pa_stream_ref, pa_stream_unref>;

using PAOperationHandle = QtPrivate::QSharedHandle<PAOperationHandleTraits>;
using PAContextHandle = QtPrivate::QSharedHandle<PAContextHandleTraits>;
using PAStreamHandle = QtPrivate::QSharedHandle<PAStreamHandleTraits>;

struct PAProplistDeleter
{
    void operator()(pa_proplist *propList) { pa_proplist_free(propList); }
};

using PAProplistHandle = std::unique_ptr<pa_proplist, PAProplistDeleter>;

struct PaMainLoopDeleter
{
    void operator()(pa_threaded_mainloop *m) const { pa_threaded_mainloop_free(m); }
};

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
