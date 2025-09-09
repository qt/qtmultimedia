// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_SPA_POD_PARSER_SUPPORT_P_H
#define QPIPEWIRE_SPA_POD_PARSER_SUPPORT_P_H

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

#include <QtCore/qdebug.h>
#include <QtCore/qspan.h>
#include <QtCore/qtconfigmacros.h>
#include <QtMultimedia/private/qaudio_qspan_support_p.h>
#include <QtMultimedia/private/qpipewire_support_p.h>

#include <spa/pod/pod.h>
#include <spa/pod/parser.h>

#include <optional>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

using QtMultimediaPrivate::drop;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
std::optional<T> spaParsePodPropertyScalar(const spa_pod &pod, unsigned spaObjectType,
                                           unsigned objectProperty)
{
    T value;
    int status = [&] {
        if constexpr (std::is_enum_v<T>) {
            return spa_pod_parse_object(&pod, spaObjectType, nullptr, objectProperty,
                                        SPA_POD_Id(&value));
        } else if constexpr (std::is_same_v<T, int>) {
            return spa_pod_parse_object(&pod, spaObjectType, nullptr, objectProperty,
                                        SPA_POD_Int(&value));
        } else {
#if !(Q_CC_GNU_ONLY && Q_CC_GNU < 1300) // P2593R1
            static_assert(false);
#endif
            Q_UNREACHABLE_RETURN(-1);
        }
    }();

    if (status == 0)
        return value;
    return std::nullopt;
}

template <typename Visitor>
auto spaVisitChoice(const spa_pod &pod, unsigned spaObjectType, unsigned objectProperty, Visitor v)
        -> decltype(v(std::declval<const spa_pod &>()))
{
    const spa_pod *format_pod = nullptr;
    int res = spa_pod_parse_object(&pod, spaObjectType, nullptr, objectProperty,
                                   SPA_POD_PodChoice(&format_pod));
    if (res < 0)
        return std::nullopt;

    if (!format_pod) {
        qWarning() << "spaVisitChoice: parse error" << pod;
        return std::nullopt;
    }

    return v(*format_pod);
}

template <typename T, spa_choice_type... Choices>
auto spaParsePodPropertyChoice(const spa_pod &pod, unsigned spaObjectType, unsigned objectProperty)
{
    constexpr bool has_choices = sizeof...(Choices) != 0;
    constexpr bool has_single_choice = sizeof...(Choices) == 1;
    constexpr bool has_enum = ((Choices == SPA_CHOICE_Enum) || ...);
    constexpr bool has_range = ((Choices == SPA_CHOICE_Range) || ...);

    // clang-format off
    using ReturnType = std::conditional_t<has_choices,
                                          std::conditional_t<has_single_choice,
                                                             std::conditional_t<has_enum,
                                                                                SpaEnum<T>,
                                                                                SpaRange<T>
                                                                               >,
                                                             std::variant<SpaEnum<T>, SpaRange<T>>
                                                            >,
                                          std::variant<SpaEnum<T>, SpaRange<T>>>;
    // clang-format on

    return spaVisitChoice(pod, spaObjectType, objectProperty,
                          [](const spa_pod &format_pod) -> std::optional<ReturnType> {
        spa_choice_type choice_type = spa_choice_type(SPA_POD_CHOICE_TYPE(&format_pod));
        if constexpr (has_enum || !has_choices) {
            if (choice_type == SPA_CHOICE_Enum)
                return SpaEnum<T>::parse(&format_pod);
        }

        if constexpr (has_range || !has_choices) {
            if (choice_type == SPA_CHOICE_Range)
                return SpaRange<T>::parse(&format_pod);
        }

        // LATER: Step/Flags
        return std::nullopt;
    });
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_SPA_POD_PARSER_SUPPORT_P_H
