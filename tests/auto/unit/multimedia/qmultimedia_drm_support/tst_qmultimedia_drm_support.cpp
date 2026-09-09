// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <private/qmultimedia_drm_support_p.h>

using namespace QtMultimediaPrivate;
using namespace Qt::StringLiterals;

namespace {

constexpr DRMModifier makeModifier(uint8_t vendor, uint64_t payload)
{
    return DRMModifier{ (uint64_t(vendor) << 56) | (payload & ((uint64_t(1) << 56) - 1)) };
}

template <typename T>
QString debugString(T value)
{
    QString result;
    QDebug(&result) << value;
    return result;
}

} // namespace

class tst_QMultimediaDrmSupport : public QObject
{
    Q_OBJECT

private slots:
    void toString_returnsFourCharacterCode_forEveryKnownDRMFormat_data();
    void toString_returnsFourCharacterCode_forEveryKnownDRMFormat();

    void debugOperator_includesToStringAndHexCode_forDRMFormat();

    void toString_decodesModifier_data();
    void toString_decodesModifier();

    void debugOperator_includesToStringAndHexCode_forDRMModifier();
};

void tst_QMultimediaDrmSupport::toString_returnsFourCharacterCode_forEveryKnownDRMFormat_data()
{
    QTest::addColumn<DRMFormat>("format");
    QTest::addColumn<QString>("expected");

    // clang-format off
    QTest::newRow("RGBA8888")    << DRMFormat::RGBA8888    << u"RA24"_s;
    QTest::newRow("RGB888")      << DRMFormat::RGB888      << u"RG24"_s;
    QTest::newRow("RG88")        << DRMFormat::RG88        << u"RG88"_s;
    QTest::newRow("ARGB8888")    << DRMFormat::ARGB8888    << u"AR24"_s;
    QTest::newRow("ABGR8888")    << DRMFormat::ABGR8888    << u"AB24"_s;
    QTest::newRow("BGRA8888")    << DRMFormat::BGRA8888    << u"BA24"_s;
    QTest::newRow("BGR888")      << DRMFormat::BGR888      << u"BG24"_s;
    QTest::newRow("GR88")        << DRMFormat::GR88        << u"GR88"_s;
    QTest::newRow("R8")          << DRMFormat::R8          << u"R8__"_s;
    QTest::newRow("R16")         << DRMFormat::R16         << u"R16_"_s;
    QTest::newRow("RGB565")      << DRMFormat::RGB565      << u"RG16"_s;
    QTest::newRow("RG1616")      << DRMFormat::RG1616      << u"RG32"_s;
    QTest::newRow("GR1616")      << DRMFormat::GR1616      << u"GR32"_s;
    QTest::newRow("BGRA1010102") << DRMFormat::BGRA1010102 << u"BA30"_s;
    QTest::newRow("YUYV")        << DRMFormat::YUYV        << u"YUYV"_s;
    QTest::newRow("UYVY")        << DRMFormat::UYVY        << u"UYVY"_s;
    QTest::newRow("AYUV")        << DRMFormat::AYUV        << u"AYUV"_s;
    QTest::newRow("NV12")        << DRMFormat::NV12        << u"NV12"_s;
    QTest::newRow("NV21")        << DRMFormat::NV21        << u"NV21"_s;
    QTest::newRow("P010")        << DRMFormat::P010        << u"P010"_s;
    QTest::newRow("YUV411")      << DRMFormat::YUV411      << u"YU11"_s;
    QTest::newRow("YUV420")      << DRMFormat::YUV420      << u"YU12"_s;
    QTest::newRow("YVU420")      << DRMFormat::YVU420      << u"YV12"_s;
    QTest::newRow("YUV422")      << DRMFormat::YUV422      << u"YU16"_s;
    QTest::newRow("YUV444")      << DRMFormat::YUV444      << u"YU24"_s;
    // clang-format on
}

void tst_QMultimediaDrmSupport::toString_returnsFourCharacterCode_forEveryKnownDRMFormat()
{
    QFETCH(DRMFormat, format);
    QFETCH(QString, expected);

    QCOMPARE_EQ(toString(format), expected);
}

void tst_QMultimediaDrmSupport::debugOperator_includesToStringAndHexCode_forDRMFormat()
{
    const QString debug = debugString(DRMFormat::NV12);

    QVERIFY(debug.contains(u"NV12"_s));
    QVERIFY(debug.contains(u"0x3231564e"_s));
}

void tst_QMultimediaDrmSupport::toString_decodesModifier_data()
{
    QTest::addColumn<DRMModifier>("modifier");
    QTest::addColumn<QString>("expectedSubstring");

    QTest::newRow("linear") << DrmFormatModifierLinear << u"linear"_s;
    QTest::newRow("invalid") << DmaBufFormatModifierInvalid << u"invalid"_s;

    // Intel: I915_FORMAT_MOD_X_TILED / Y_TILED
    QTest::newRow("intel-x-tiled") << makeModifier(0x01, 1) << u"Intel, X tiled"_s;
    QTest::newRow("intel-y-tiled-ccs") << makeModifier(0x01, 4) << u"Intel, Y tiled, CCS"_s;
    QTest::newRow("intel-unknown") << makeModifier(0x01, 0xff) << u"Intel, unknown layout"_s;

    // AMD: AMD_FMT_MOD_TILE_VER_GFX9, AMD_FMT_MOD_TILE_GFX9_64K_S, no DCC
    QTest::newRow("amd-gfx9-no-dcc")
            << makeModifier(0x02, 1 | (9 << 8)) << u"AMD, tile version GFX9, tile mode 9"_s;
    // same, with DCC + DCC_RETILE set
    QTest::newRow("amd-gfx9-dcc-retile")
            << makeModifier(0x02, 1 | (9 << 8) | (1 << 13) | (1 << 14))
            << u"DCC (retile: yes, pipe-aligned: no, max compressed block: 64B)"_s;

    // NVIDIA: DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_FOUR_GOB == BLOCK_LINEAR_2D(0,0,0,0,2)
    QTest::newRow("nvidia-block-linear")
            << makeModifier(0x03, 0x10 | 2) << u"block height 2^2 GOBs"_s;
    QTest::newRow("nvidia-tegra-tiled") << makeModifier(0x03, 1) << u"NVIDIA, Tegra tiled"_s;

    // Samsung: DRM_FORMAT_MOD_SAMSUNG_64_32_TILE / 16_16_TILE
    QTest::newRow("samsung-64-32") << makeModifier(0x04, 1) << u"Samsung, 64x32 tiled"_s;
    QTest::newRow("samsung-16-16") << makeModifier(0x04, 2) << u"Samsung, 16x16 tiled"_s;

    // Qualcomm: DRM_FORMAT_MOD_QCOM_COMPRESSED / TILED2 / TILED3
    QTest::newRow("qcom-compressed") << makeModifier(0x05, 1) << u"Qualcomm, compressed"_s;
    QTest::newRow("qcom-tiled2") << makeModifier(0x05, 2) << u"Qualcomm, alternate tiled"_s;
    QTest::newRow("qcom-tiled3") << makeModifier(0x05, 3) << u"Qualcomm, tiled"_s;

    // Vivante: DRM_FORMAT_MOD_VIVANTE_TILED / SUPER_TILED
    QTest::newRow("vivante-tiled") << makeModifier(0x06, 1) << u"Vivante, 4x4 tiled"_s;
    QTest::newRow("vivante-super-tiled")
            << makeModifier(0x06, 2) << u"Vivante, 64x64 super-tiled"_s;

    // Broadcom: DRM_FORMAT_MOD_BROADCOM_VC4_T_TILED / SAND128_COL_HEIGHT(42) / UIF
    QTest::newRow("broadcom-vc4-t-tiled") << makeModifier(0x07, 1) << u"Broadcom, VC4 T-tiled"_s;
    QTest::newRow("broadcom-sand128")
            << makeModifier(0x07, 4 | (42ULL << 8)) << u"SAND128, column height 42"_s;
    QTest::newRow("broadcom-uif") << makeModifier(0x07, 6) << u"Broadcom, UIF"_s;

    // ARM: DRM_FORMAT_MOD_ARM_AFBC(AFBC_FORMAT_MOD_BLOCK_SIZE_16x16 | AFBC_FORMAT_MOD_SPARSE)
    QTest::newRow("arm-afbc") << makeModifier(0x08, 1 | (1ULL << 6))
                              << u"ARM, AFBC, superblock size 16x16, flags: sparse"_s;

    // Allwinner: DRM_FORMAT_MOD_ALLWINNER_TILED
    QTest::newRow("allwinner-tiled") << makeModifier(0x09, 1) << u"Allwinner, tiled"_s;

    // Amlogic: DRM_FORMAT_MOD_AMLOGIC_FBC(BASIC, MEM_SAVING)
    QTest::newRow("amlogic-fbc-basic-memsaving")
            << makeModifier(0x0a, 1 | (1 << 8)) << u"Amlogic, FBC basic layout, memory saving"_s;

    // unknown vendor: falls back to raw payload
    QTest::newRow("unknown-vendor") << makeModifier(0x0b, 0x1234) << u"unknown vendor 0x0b"_s;
}

void tst_QMultimediaDrmSupport::toString_decodesModifier()
{
    QFETCH(DRMModifier, modifier);
    QFETCH(QString, expectedSubstring);

    const QString result = toString(modifier);
    QVERIFY2(result.contains(expectedSubstring),
             qPrintable(u"toString() == '%1', expected to contain '%2'"_s.arg(result,
                                                                              expectedSubstring)));
}

void tst_QMultimediaDrmSupport::debugOperator_includesToStringAndHexCode_forDRMModifier()
{
    const QString debug = debugString(DrmFormatModifierLinear);

    QVERIFY(debug.contains(u"linear"_s));
    QVERIFY(debug.contains(u"0x0"_s));
}

QTEST_MAIN(tst_QMultimediaDrmSupport)
#include "tst_qmultimedia_drm_support.moc"
