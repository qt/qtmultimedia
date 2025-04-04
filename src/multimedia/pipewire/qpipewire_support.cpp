// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_support_p.h"

#include <QtCore/qspan.h>
#include <QtMultimedia/private/qmultimedia_enum_to_string_converter_p.h>

#include <pipewire/version.h>
#include <spa/debug/pod.h>

#include <cstdarg>

QT_BEGIN_NAMESPACE

// debug support
QDebug operator<<(QDebug dbg, const spa_dict &dict)
{
    QSpan<const spa_dict_item> items{ dict.items, dict.n_items };

    QDebugStateSaver saver(dbg);
    dbg.nospace();

    for (const spa_dict_item &item : items)
        dbg << item.key << "=" << item.value << ", ";
    return dbg;
}

#if PW_CHECK_VERSION(0, 3, 65)

namespace {

struct QtSpaDebugContext : ::spa_debug_context
{
    explicit QtSpaDebugContext(QDebug dbg)
        : ::spa_debug_context{},
          dbg{
              std::move(dbg),
          }
    {
        ::spa_debug_context::log = &doLog;
    }

    static void doLog(::spa_debug_context *ctx, const char *fmt, ...)
    {
        std::va_list args;
        va_start(args, fmt);
        QString logString = QString::vasprintf(fmt, args);
        va_end(args);

        auto *self = static_cast<QtSpaDebugContext *>(ctx);
        self->dbg << logString;
    }

    QDebug dbg;
};

} // namespace

QDebug operator<<(QDebug dbg, const spa_pod &pod)
{
    QtSpaDebugContext context{ std::move(dbg) };

    spa_debugc_pod(&context, 0, nullptr, &pod);
    return std::move(context.dbg);
}

#else

QDebug operator<<(QDebug dbg, const spa_pod &)
{
    dbg << "QDebug operator<<(QDebug, const spa_pod &) not implemented: minimum requirement: "
           "pipewire-0.3.65";
    return dbg;
}

#endif

// clang-format off
QT_MM_MAKE_STRING_RESOLVER( pw_stream_state, QtMultimediaPrivate::EnumName,
                           (PW_STREAM_STATE_ERROR, "error")
                           (PW_STREAM_STATE_UNCONNECTED, "unconnected")
                           (PW_STREAM_STATE_CONNECTING, "connecting")
                           (PW_STREAM_STATE_PAUSED, "paused")
                           (PW_STREAM_STATE_STREAMING, "streaming")
                          );
// clang-format on

QT_MM_DEFINE_QDEBUG_ENUM(pw_stream_state);

QDebug operator<<(QDebug dbg, const pw_time &state)
{
    // pw_time may have more members, but those are only required to exist in 0.3.55 and later
    dbg << QStringLiteral(u"pw_time{now: %1ns, rate: %2/%3, ticks: %4, delay: %5, queued: %6}")
                    .arg(state.now)
                    .arg(state.rate.num)
                    .arg(state.rate.denom)
                    .arg(state.ticks)
                    .arg(state.delay)
                    .arg(state.queued);
    return dbg;
}

QT_END_NAMESPACE
