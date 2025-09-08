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

#include <spa/param/audio/raw.h>
#include <spa/pod/pod.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

using QtMultimediaPrivate::drop;
using QtMultimediaPrivate::take;

template <typename T>
struct SpaRange
{
    static std::optional<SpaRange> parse(const struct spa_pod *value)
    {
        if (SPA_POD_CHOICE_N_VALUES(value) != 3)
            return std::nullopt;

        T *v = reinterpret_cast<T *>(SPA_POD_CHOICE_VALUES(value));

        return SpaRange{
            .defaultValue = v[0],
            .minValue = v[1],
            .maxValue = v[2],
        };
    }

    T defaultValue;
    T minValue;
    T maxValue;
};

template <typename T>
struct SpaEnum
{
    static std::optional<SpaEnum> parse(const struct spa_pod *value)
    {
        int numberOfChoices = SPA_POD_CHOICE_N_VALUES(value);

        if (SPA_POD_CHOICE_N_VALUES(value) < 1)
            return std::nullopt;

        QSpan<const T> values{
            reinterpret_cast<const T *>(SPA_POD_CHOICE_VALUES(value)),
            numberOfChoices,
        };

        return SpaEnum{ values };
    }

    const T &defaultValue() const
    {
        Q_ASSERT(!m_values.empty());
        return m_values.front();
    }

    QSpan<const T> values() const
    {
        Q_ASSERT(m_values.size() > 1);
        return drop(QSpan{ m_values }, 1);
    }

private:
    explicit SpaEnum(QSpan<const T> args)
        : m_values{
              args.begin(),
              args.end(),
          }
    {
    }

    std::vector<T> m_values;
};

struct SpaObjectAudioFormat
{
    static std::optional<SpaObjectAudioFormat> parse(const struct spa_pod_object *obj);
    static std::optional<SpaObjectAudioFormat> parse(const struct spa_pod *pod);

    int channelCount = 0;
    std::variant<int, std::vector<int>, SpaRange<int>> rates;
    std::variant<spa_audio_format, SpaEnum<spa_audio_format>> sampleTypes;
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
