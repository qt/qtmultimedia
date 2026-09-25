// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMATERIALIZINGVIDEOBUFFER_P_H
#define QMATERIALIZINGVIDEOBUFFER_P_H

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

#include <QtMultimedia/qabstractvideobuffer.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/private/qcaptureslotregistry_p.h>
#include <QtMultimedia/private/qhwvideobuffer_p.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

#include <array>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

#ifdef __cpp_concepts
#  include <concepts>
#endif

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

#ifdef __cpp_concepts
// clang-format off
template <typename Source>
concept MaterializableCaptureSource =
        std::default_initializable<Source> && std::movable<Source>
        && requires(const Source &source, QAbstractVideoBuffer::MapData &mapData) {
            { source.mapReadOnly() }
                -> std::same_as<std::optional<QAbstractVideoBuffer::MapData>>;
            { source.unmap(mapData) } -> std::same_as<void>;
        };

template <typename Registry>
concept MaterializingRegistry =
        requires(Registry &registry, typename Registry::SlotType slot) {
            typename Registry::SlotType;
            typename Registry::Lease;
            requires CaptureSlot<typename Registry::SlotType>;
            { registry.tryReserveSlot(std::move(slot)) }
                -> std::same_as<std::optional<typename Registry::Lease>>;
        };

template <typename BufferType>
concept HwBufferType = std::derived_from<BufferType, QHwVideoBuffer>;
// clang-format on
#endif

#ifdef __cpp_concepts
template <MaterializableCaptureSource Source>
#else
template <typename Source>
#endif
class MaterializableSource final
{
public:
    Q_DISABLE_COPY_MOVE(MaterializableSource)

    explicit MaterializableSource(Source source) : m_source(std::move(source)) { }

    ~MaterializableSource()
    {
        {
            std::lock_guard lock(m_mutex);
            if (m_mappingState == MappingState::SourceMapped) {
                m_source.unmap(m_sourceMapping);
                m_sourceMapping = {};
                m_mappingState = MappingState::None;
            }
        }
    }

    QAbstractVideoBuffer::MapData map(QVideoFrame::MapMode mode)
    {
        std::lock_guard lock(m_mutex);

        switch (mode) {
        case QVideoFrame::ReadOnly: {
            // If already materialized, serve the copy. But keep SourceMapped mapped if it was.
            if (m_materialized) {
                qCDebug(qLcCaptureSlotsTrace) << "returning materialized mapping";
                return materializedMapData(mode);
            }

            // Map the source on first read-only access, or re-serve if already mapped.
            if (m_mappingState == MappingState::None) {
                std::optional<QAbstractVideoBuffer::MapData> mapping = m_source.mapReadOnly();
                if (!mapping) {
                    qCWarning(qLcCaptureSlots) << "mapping producer memory failed";
                    return {};
                }
                m_sourceMapping = *mapping;
                m_mappingState = MappingState::SourceMapped;
                qCDebug(qLcCaptureSlotsTrace)
                        << "mapped source:" << m_sourceMapping.planeCount << "planes";
            } else {
                Q_ASSERT(m_mappingState == MappingState::SourceMapped);
                qCDebug(qLcCaptureSlotsTrace) << "returning stored source mapping";
            }
            return m_sourceMapping;
        }
        case QVideoFrame::WriteOnly:
        case QVideoFrame::ReadWrite:
            // Writing into the producer's capture buffer is not on: take a copy first.
            qCDebug(qLcCaptureSlotsTrace) << "write map: materializing first";
            materializeLocked();
            // Never overwrite an outstanding source mapping: unmap() must still end it.
            if (m_mappingState == MappingState::None)
                m_mappingState = MappingState::Copy;
            return materializedMapData(mode);

        default:
            Q_UNREACHABLE_RETURN({});
        }
    }

    void unmap()
    {
        std::lock_guard lock(m_mutex);
        if (m_mappingState == MappingState::SourceMapped) {
            qCDebug(qLcCaptureSlotsTrace) << "unmapping source mapping";
            m_source.unmap(m_sourceMapping);
            m_sourceMapping = {};
            m_mappingState = MappingState::None;
            releaseSourceIfUnused();
        }
    }

    // Copy the data out, after which the producer's memory is never touched again.
    void materialize()
    {
        std::lock_guard lock(m_mutex);
        materializeLocked();
    }

    bool isMaterialized() const
    {
        std::lock_guard lock(m_mutex);
        return m_materialized;
    }

private:
    enum class MappingState : uint8_t {
        None, // No mapping or materialization yet.
        SourceMapped, // Source is currently mapped by mapReadOnly().
        Copy, // Materialized copy is available.
    };

    void materializeLocked()
    {
        if (m_materialized) {
            qCDebug(qLcCaptureSlotsTrace) << "already materialized";
            return;
        }

        // Copy out of the consumer's existing mapping rather than mapping a second time, and leave
        // it mapped -- they may be holding pointers into it.
        QAbstractVideoBuffer::MapData source{};
        std::optional<QAbstractVideoBuffer::MapData> freshMapping;
        if (m_mappingState == MappingState::SourceMapped) {
            source = m_sourceMapping;
        } else {
            freshMapping = m_source.mapReadOnly();
            if (freshMapping) {
                source = *freshMapping;
            } else {
                qCWarning(qLcCaptureSlots) << "mapping producer memory for copy-out failed";
            }
        }

        qsizetype totalSize = 0;
        for (int i = 0; i < source.planeCount; ++i)
            totalSize += source.dataSize[i];

        m_data.resizeForOverwrite(totalSize);
        m_materializedMapping = {};
        m_materializedMapping.planeCount = source.planeCount;

        qsizetype offset = 0;
        for (int i = 0; i < source.planeCount; ++i) {
            std::memcpy(m_data.data() + offset, source.data[i], size_t(source.dataSize[i]));
            m_planeOffsets[i] = offset;
            m_materializedMapping.bytesPerLine[i] = source.bytesPerLine[i];
            m_materializedMapping.dataSize[i] = source.dataSize[i];
            offset += source.dataSize[i];
        }
        qCDebug(qLcCaptureSlotsTrace)
                << "materialized" << source.planeCount << "planes," << totalSize << "bytes";

        if (freshMapping)
            m_source.unmap(*freshMapping);

        m_materialized = true;
        releaseSourceIfUnused();
    }

    // The source is only still needed to undo an outstanding mapping.
    void releaseSourceIfUnused()
    {
        if (m_materialized && m_mappingState != MappingState::SourceMapped)
            m_source = Source{};
    }

    QAbstractVideoBuffer::MapData materializedMapData(QVideoFrame::MapMode mode)
    {
        QAbstractVideoBuffer::MapData mapData = m_materializedMapping;
        auto *base = mode == QVideoFrame::ReadOnly
                ? reinterpret_cast<uchar *>(const_cast<char *>(m_data.constData()))
                : reinterpret_cast<uchar *>(m_data.data());
        for (int i = 0; i < mapData.planeCount; ++i)
            mapData.data[i] = base + m_planeOffsets[i];
        return mapData;
    }

    mutable QMutex m_mutex;

    Source m_source;
    QAbstractVideoBuffer::MapData m_sourceMapping;
    MappingState m_mappingState = MappingState::None;

    bool m_materialized = false;
    QByteArray m_data;
    QAbstractVideoBuffer::MapData m_materializedMapping;
    std::array<qsizetype, 4> m_planeOffsets{};
};

// A video buffer wrapping producer-owned capture memory, holding the producer's slot for as long
// as any QVideoFrame references it.
#ifdef __cpp_concepts
template <MaterializableCaptureSource Source, MaterializingRegistry Registry,
          HwBufferType Base = QHwVideoBuffer>
#else
template <typename Source, typename Registry, typename Base = QHwVideoBuffer>
#endif
class MaterializingVideoBuffer final : public Base
{
    static_assert(std::is_base_of_v<QAbstractVideoBuffer, Base>);

public:
    using State = MaterializableSource<Source>;
    using SlotType = typename Registry::SlotType;
    using Lease = typename Registry::Lease;

    // On refusal, slot is left intact for the caller's eager-copy fallback.
    template <typename SlotArg, typename... BaseArgs>
    static std::unique_ptr<MaterializingVideoBuffer>
    tryCreate(Registry &registry, SlotArg &&slot, Source source, QVideoFrameFormat format = {},
              BaseArgs &&...baseArgs)
    {
        std::optional lease = registry.tryReserveSlot(std::forward<SlotArg>(slot));
        if (!lease) {
            qCDebug(qLcCaptureSlotsTrace) << "pool under pressure, caller must copy eagerly";
            return nullptr;
        }

        return std::make_unique<MaterializingVideoBuffer>(std::move(*lease), std::move(source),
                                                          std::move(format),
                                                          std::forward<BaseArgs>(baseArgs)...);
    }

    Q_DISABLE_COPY_MOVE(MaterializingVideoBuffer)

    template <typename... BaseArgs>
    MaterializingVideoBuffer(Lease lease, Source source, QVideoFrameFormat format = {},
                             BaseArgs &&...baseArgs)
        : Base(std::forward<BaseArgs>(baseArgs)...),
          m_lease(std::move(lease)),
          m_state(std::move(source)),
          m_format(std::move(format))
    {
    }

    QAbstractVideoBuffer::MapData map(QVideoFrame::MapMode mode) override
    {
        return m_state.map(mode);
    }
    void unmap() override { m_state.unmap(); }

    State &state() noexcept { return m_state; }

    template <typename... BaseArgs>
    static std::unique_ptr<QAbstractVideoBuffer>
    wrapOrMaterialize(Registry &registry, SlotType slot, Source source,
                      QVideoFrameFormat format = {}, BaseArgs &&...baseArgs);

private:
    const Lease m_lease;
    State m_state;
    const QVideoFrameFormat m_format;
};

// The eager-copy counterpart of MaterializingVideoBuffer: copies the producer's memory up front
// and holds no capture slot. This is what a producer falls back to when back-pressure refuses a
// slot, or when the format has to be copied anyway.
#ifdef __cpp_concepts
template <MaterializableCaptureSource Source>
#else
template <typename Source>
#endif
class MaterializedVideoBuffer final : public QAbstractVideoBuffer
{
public:
    Q_DISABLE_COPY_MOVE(MaterializedVideoBuffer)

    explicit MaterializedVideoBuffer(Source source, QVideoFrameFormat format = {})
        : m_state(std::move(source)), m_format(std::move(format))
    {
        m_state.materialize();
        qCDebug(qLcCaptureSlotsTrace) << "copied producer memory eagerly";
    }

    MapData map(QVideoFrame::MapMode mode) override { return m_state.map(mode); }
    void unmap() override { m_state.unmap(); }
    QVideoFrameFormat format() const override { return m_format; }

private:
    // Not const: map()/unmap() drive the state machine.
    MaterializableSource<Source> m_state;
    const QVideoFrameFormat m_format;
};

#ifdef __cpp_concepts
template <MaterializableCaptureSource Source, MaterializingRegistry Registry, HwBufferType Base>
#else
template <typename Source, typename Registry, typename Base>
#endif
template <typename... BaseArgs>
inline std::unique_ptr<QAbstractVideoBuffer>
MaterializingVideoBuffer<Source, Registry, Base>::wrapOrMaterialize(Registry &registry,
                                                                    SlotType slot, Source source,
                                                                    QVideoFrameFormat format,
                                                                    BaseArgs &&...baseArgs)
{
    if (auto ret = tryCreate(registry, std::move(slot), source, format,
                             std::forward<BaseArgs>(baseArgs)...))
        return ret;

    auto ret =
            std::make_unique<MaterializedVideoBuffer<Source>>(std::move(source), std::move(format));
    registry.recycleSlot(std::move(slot));
    return ret;
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QMATERIALIZINGVIDEOBUFFER_P_H
