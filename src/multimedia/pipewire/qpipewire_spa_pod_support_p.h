// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_SPA_POD_SUPPORT_P_H
#define QPIPEWIRE_SPA_POD_SUPPORT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qlist.h>
#include <QtCore/qspan.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/private/qaudio_qspan_support_p.h>
#include <QtMultimedia/private/qpipewire_spa_pod_parser_support_p.h>

#if __has_include(<spa/param/audio/iec958.h>)
#  include <spa/param/audio/iec958.h>
#else
#  include "qpipewire_spa_compat_p.h"
#endif
#include <spa/param/audio/raw.h>
#include <spa/pod/pod.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

struct SpaObjectAudioFormat
{
    static std::optional<SpaObjectAudioFormat> parse(const struct spa_pod_object *obj);
    static std::optional<SpaObjectAudioFormat> parse(const struct spa_pod *pod);

    int channelCount = 0;
    std::variant<int, std::vector<int>, SpaRange<int>> rates;
    std::variant<spa_audio_format, SpaEnum<spa_audio_format>, spa_audio_iec958_codec> sampleTypes;
    std::optional<QList<spa_audio_channel>> channelPositions; // COW-able
};

spa_audio_info_raw asSpaAudioInfoRaw(const QAudioFormat &);

inline constexpr std::array channelPositionsMono{ SPA_AUDIO_CHANNEL_MONO };
inline constexpr std::array channelPositionsStereo{ SPA_AUDIO_CHANNEL_FL, SPA_AUDIO_CHANNEL_FR };
inline constexpr std::array channelPositions2Dot1{ SPA_AUDIO_CHANNEL_FL, SPA_AUDIO_CHANNEL_FR,
                                                   SPA_AUDIO_CHANNEL_LFE };
inline constexpr std::array channelPositions3Dot0{ SPA_AUDIO_CHANNEL_FL, SPA_AUDIO_CHANNEL_FR,
                                                   SPA_AUDIO_CHANNEL_FC };
inline constexpr std::array channelPositions3Dot1{ SPA_AUDIO_CHANNEL_FL, SPA_AUDIO_CHANNEL_FR,
                                                   SPA_AUDIO_CHANNEL_FC, SPA_AUDIO_CHANNEL_LFE };
inline constexpr std::array channelPositions5Dot0{ SPA_AUDIO_CHANNEL_FL, SPA_AUDIO_CHANNEL_FR,
                                                   SPA_AUDIO_CHANNEL_FC, SPA_AUDIO_CHANNEL_RL,
                                                   SPA_AUDIO_CHANNEL_RR };
inline constexpr std::array channelPositions5Dot1{ SPA_AUDIO_CHANNEL_FL, SPA_AUDIO_CHANNEL_FR,
                                                   SPA_AUDIO_CHANNEL_FC, SPA_AUDIO_CHANNEL_LFE,
                                                   SPA_AUDIO_CHANNEL_RL, SPA_AUDIO_CHANNEL_RR };
inline constexpr std::array channelPositions7Dot0{ SPA_AUDIO_CHANNEL_FL, SPA_AUDIO_CHANNEL_FR,
                                                   SPA_AUDIO_CHANNEL_FC, SPA_AUDIO_CHANNEL_RL,
                                                   SPA_AUDIO_CHANNEL_RR, SPA_AUDIO_CHANNEL_SL,
                                                   SPA_AUDIO_CHANNEL_SR };
inline constexpr std::array channelPositions7Dot1{ SPA_AUDIO_CHANNEL_FL, SPA_AUDIO_CHANNEL_FR,
                                                   SPA_AUDIO_CHANNEL_FC, SPA_AUDIO_CHANNEL_LFE,
                                                   SPA_AUDIO_CHANNEL_RL, SPA_AUDIO_CHANNEL_RR,
                                                   SPA_AUDIO_CHANNEL_SL, SPA_AUDIO_CHANNEL_SR };

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_SPA_POD_SUPPORT_P_H
