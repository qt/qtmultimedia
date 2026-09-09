// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmultimedia_drm_support_p.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QtMultimediaPrivate {

namespace {

// see DRM_FORMAT_MOD_VENDOR_* in drm_fourcc.h
QString modifierVendorName(uint8_t vendor)
{
    switch (vendor) {
    case 0x00:
        return u"none"_s;
    case 0x01:
        return u"Intel"_s;
    case 0x02:
        return u"AMD"_s;
    case 0x03:
        return u"NVIDIA"_s;
    case 0x04:
        return u"Samsung"_s;
    case 0x05:
        return u"Qualcomm"_s;
    case 0x06:
        return u"Vivante"_s;
    case 0x07:
        return u"Broadcom"_s;
    case 0x08:
        return u"ARM"_s;
    case 0x09:
        return u"Allwinner"_s;
    case 0x0a:
        return u"Amlogic"_s;
    default:
        return u"unknown vendor 0x%1"_s.arg(vendor, 2, 16, u'0');
    }
}

// Decodes DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(c, s, g, k, h) per the bitfield
// layout documented in drm_fourcc.h.
QString decodeNvidiaModifier(uint64_t payload)
{
    if (payload == 1)
        return u"Tegra tiled"_s;
    if (!(payload & 0x10))
        return u"unknown layout, raw 0x%1"_s.arg(payload, 14, 16, u'0');

    const uint32_t h = payload & 0xf;
    const uint32_t k = (payload >> 12) & 0xff;
    const uint32_t g = (payload >> 20) & 0x3;
    const uint32_t s = (payload >> 22) & 0x1;
    const uint32_t c = (payload >> 23) & 0x7;

    static const QString gobHeights[] = {
        u"8, Fermi-Volta/Tegra K1+ page kind mapping"_s,
        u"4, G80-GT2XX page kind mapping"_s,
        u"8, Turing+ page kind mapping"_s,
        u"reserved"_s,
    };
    static const QString compressionKinds[] = {
        u"none"_s,         u"ROP/3D layout 1"_s, u"ROP/3D layout 2"_s, u"CDE horizontal"_s,
        u"CDE vertical"_s, u"reserved"_s,        u"reserved"_s,        u"reserved"_s,
    };

    return u"block-linear, block height 2^%1 GOBs, page kind 0x%2, GOB height %3, "
           "sector layout: %4, compression: %5"_s.arg(h)
                   .arg(k, 2, 16, u'0')
                   .arg(gobHeights[g])
                   .arg(s ? u"desktop GPU / Tegra Xavier+"_s : u"Tegra K1 - Parker/TX2"_s)
                   .arg(compressionKinds[c]);
}

// see I915_FORMAT_MOD_* in drm_fourcc.h
QString decodeIntelModifier(uint64_t payload)
{
    switch (payload) {
    case 1:
        return u"X tiled"_s;
    case 2:
        return u"Y tiled"_s;
    case 3:
        return u"Yf tiled"_s;
    case 4:
        return u"Y tiled, CCS"_s;
    case 5:
        return u"Yf tiled, CCS"_s;
    case 6:
        return u"Y tiled, Gen12 render compression CCS"_s;
    case 7:
        return u"Y tiled, Gen12 media compression CCS"_s;
    case 8:
        return u"Y tiled, Gen12 render compression CCS with clear color"_s;
    case 9:
        return u"4 tiled"_s;
    case 10:
        return u"4 tiled, DG2 render compression CCS"_s;
    case 11:
        return u"4 tiled, DG2 media compression CCS"_s;
    case 12:
        return u"4 tiled, DG2 render compression CCS with clear color"_s;
    default:
        return u"unknown layout, raw 0x%1"_s.arg(payload, 14, 16, u'0');
    }
}

// see AMD_FMT_MOD_* in drm_fourcc.h
QString decodeAmdModifier(uint64_t payload)
{
    const uint32_t tileVersion = payload & 0xff;
    if (tileVersion == 0)
        return u"GFX8 or older, linear-compatible"_s;

    static const QString tileVersions[] = {
        {}, u"GFX9"_s, u"GFX10"_s, u"GFX10 RBPlus"_s, u"GFX11"_s,
    };
    static const QString dccBlockSizes[] = {
        u"64B"_s,
        u"128B"_s,
        u"256B"_s,
        u"reserved"_s,
    };

    const uint32_t tile = (payload >> 8) & 0x1f;
    const bool dcc = (payload >> 13) & 0x1;

    QString result = u"tile version %1, tile mode %2"_s
                             .arg(tileVersion < std::size(tileVersions) ? tileVersions[tileVersion]
                                                                        : u"unknown"_s)
                             .arg(tile);
    if (dcc) {
        const bool dccRetile = (payload >> 14) & 0x1;
        const bool dccPipeAlign = (payload >> 15) & 0x1;
        const uint32_t dccMaxCompressedBlock = (payload >> 18) & 0x3;
        result += u", DCC (retile: %1, pipe-aligned: %2, max compressed block: %3)"_s
                          .arg(dccRetile ? u"yes"_s : u"no"_s)
                          .arg(dccPipeAlign ? u"yes"_s : u"no"_s)
                          .arg(dccBlockSizes[dccMaxCompressedBlock]);
    }
    return result;
}

// see DRM_FORMAT_MOD_ARM_* and AFBC_FORMAT_MOD_* in drm_fourcc.h
QString decodeArmModifier(uint64_t payload)
{
    const uint32_t type = (payload >> 52) & 0xf;
    const uint64_t value = payload & 0x000fffffffffffffULL;

    if (type != 0x00) // DRM_FORMAT_MOD_ARM_TYPE_AFBC
        return u"type 0x%1, raw 0x%2"_s.arg(type).arg(value, 13, 16, u'0');

    static const QString blockSizes[] = {
        u"none"_s, u"16x16"_s, u"32x8"_s, u"64x4"_s, u"32x8_64x4"_s,
    };
    const uint32_t blockSize = value & 0xf;

    QStringList flags;
    if (value & (1ULL << 4))
        flags << u"YTR"_s;
    if (value & (1ULL << 5))
        flags << u"split"_s;
    if (value & (1ULL << 6))
        flags << u"sparse"_s;
    if (value & (1ULL << 7))
        flags << u"CBR"_s;
    if (value & (1ULL << 8))
        flags << u"tiled"_s;
    if (value & (1ULL << 9))
        flags << u"SC"_s;
    if (value & (1ULL << 10))
        flags << u"double-buffered"_s;
    if (value & (1ULL << 11))
        flags << u"block-channel"_s;
    if (value & (1ULL << 12))
        flags << u"USM"_s;

    return u"AFBC, superblock size %1%2"_s
            .arg(blockSize < std::size(blockSizes) ? blockSizes[blockSize] : u"unknown"_s)
            .arg(flags.isEmpty() ? QString() : u", flags: "_s + flags.join(u'|'));
}

// see DRM_FORMAT_MOD_BROADCOM_* in drm_fourcc.h
QString decodeBroadcomModifier(uint64_t payload)
{
    const uint64_t mod = payload & 0xff;
    const uint64_t param = (payload >> 8) & 0xffffffffffffULL;

    switch (mod) {
    case 1:
        return u"VC4 T-tiled"_s;
    case 2:
        return u"SAND32, column height %1"_s.arg(param);
    case 3:
        return u"SAND64, column height %1"_s.arg(param);
    case 4:
        return u"SAND128, column height %1"_s.arg(param);
    case 5:
        return u"SAND256, column height %1"_s.arg(param);
    case 6:
        return u"UIF"_s;
    default:
        return u"unknown layout, raw 0x%1"_s.arg(payload, 14, 16, u'0');
    }
}

// see DRM_FORMAT_MOD_QCOM_* in drm_fourcc.h
QString decodeQualcommModifier(uint64_t payload)
{
    switch (payload) {
    case 1:
        return u"compressed"_s;
    case 2:
        return u"alternate tiled"_s;
    case 3:
        return u"tiled"_s;
    default:
        return u"unknown layout, raw 0x%1"_s.arg(payload, 14, 16, u'0');
    }
}

// see DRM_FORMAT_MOD_VIVANTE_* in drm_fourcc.h
QString decodeVivanteModifier(uint64_t payload)
{
    switch (payload) {
    case 1:
        return u"4x4 tiled"_s;
    case 2:
        return u"64x64 super-tiled"_s;
    case 3:
        return u"4x4 tiled, split (dual-pipe)"_s;
    case 4:
        return u"64x64 super-tiled, split (dual-pipe)"_s;
    default:
        return u"unknown layout, raw 0x%1"_s.arg(payload, 14, 16, u'0');
    }
}

// see DRM_FORMAT_MOD_SAMSUNG_* in drm_fourcc.h
QString decodeSamsungModifier(uint64_t payload)
{
    switch (payload) {
    case 1:
        return u"64x32 tiled"_s;
    case 2:
        return u"16x16 tiled"_s;
    default:
        return u"unknown layout, raw 0x%1"_s.arg(payload, 14, 16, u'0');
    }
}

// see DRM_FORMAT_MOD_ALLWINNER_TILED in drm_fourcc.h
QString decodeAllwinnerModifier(uint64_t payload)
{
    if (payload == 1)
        return u"tiled"_s;
    return u"unknown layout, raw 0x%1"_s.arg(payload, 14, 16, u'0');
}

// see DRM_FORMAT_MOD_AMLOGIC_FBC in drm_fourcc.h
QString decodeAmlogicModifier(uint64_t payload)
{
    const uint32_t layout = payload & 0xff;
    const uint32_t options = (payload >> 8) & 0xff;

    QString result;
    switch (layout) {
    case 1:
        result = u"FBC basic layout"_s;
        break;
    case 2:
        result = u"FBC scatter layout"_s;
        break;
    default:
        return u"unknown layout, raw 0x%1"_s.arg(payload, 14, 16, u'0');
    }

    if (options & 0x1)
        result += u", memory saving"_s;
    return result;
}

} // namespace

QString toString(DRMFormat format)
{
    const uint32_t code = qToUnderlying(format);
    QString result;
    for (int i = 0; i < 4; ++i) {
        const char c = char((code >> (8 * i)) & 0xff);
        result += QChar(c == ' ' ? u'_' : c);
    }
    return result;
}

QDebug operator<<(QDebug debug, DRMFormat format)
{
    const QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "DRMFormat(0x" << Qt::hex << qToUnderlying(format) << Qt::dec
                              << ", " << toString(format) << ')';
    return debug;
}

QString toString(DRMModifier modifier)
{
    if (modifier == DrmFormatModifierLinear)
        return u"linear"_s;
    if (modifier == DmaBufFormatModifierInvalid)
        return u"invalid"_s;

    const uint64_t value = qToUnderlying(modifier);
    const uint8_t vendor = uint8_t(value >> 56);
    const uint64_t payload = value & ((uint64_t(1) << 56) - 1);
    const QString vendorName = modifierVendorName(vendor);

    switch (vendor) {
    case 0x01:
        return vendorName + u", "_s + decodeIntelModifier(payload);
    case 0x02:
        return vendorName + u", "_s + decodeAmdModifier(payload);
    case 0x03:
        return vendorName + u", "_s + decodeNvidiaModifier(payload);
    case 0x04:
        return vendorName + u", "_s + decodeSamsungModifier(payload);
    case 0x05:
        return vendorName + u", "_s + decodeQualcommModifier(payload);
    case 0x06:
        return vendorName + u", "_s + decodeVivanteModifier(payload);
    case 0x07:
        return vendorName + u", "_s + decodeBroadcomModifier(payload);
    case 0x08:
        return vendorName + u", "_s + decodeArmModifier(payload);
    case 0x09:
        return vendorName + u", "_s + decodeAllwinnerModifier(payload);
    case 0x0a:
        return vendorName + u", "_s + decodeAmlogicModifier(payload);
    default:
        return u"%1, vendor-specific payload 0x%2"_s.arg(vendorName).arg(payload, 14, 16, u'0');
    }
}

QDebug operator<<(QDebug debug, DRMModifier modifier)
{
    const QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "DRMModifier(0x" << Qt::hex << qToUnderlying(modifier) << Qt::dec
                              << ", " << toString(modifier) << ')';
    return debug;
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
