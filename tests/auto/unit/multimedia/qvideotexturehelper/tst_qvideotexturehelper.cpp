// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qbytearray.h>
#include <QtTest/qtest.h>

#include <private/qvideotexturehelper_p.h>
#include <qvideoframe.h>

#include "qvideoframeformat.h"

QT_USE_NAMESPACE

struct ColorSpaceCoeff
{
    float a;
    float b;
    float c;
    float d;
    float e;
};

// Coefficients used in ITU-R BT.709-6 Table 3 - Signal format
constexpr ColorSpaceCoeff BT709Coefficients = {
    0.2126f, 0.7152f, 0.0722f, // E_g' = 0.2126 * E_R' + 0.7152 * E_G' + 0.0722 * E_B'
    1.8556f,                   // E_CB' = (E_B' - E_g') / 1.8556
    1.5748f,                   // E_CR' = (E_R' - E_g') / 1.5748
};

// Coefficients used in ITU-R BT.2020-2 Table 4 - Signal format
constexpr ColorSpaceCoeff BT2020Coefficients = {
    0.2627f, 0.6780f, 0.0593f, // Y_c' = (0.2627 R + 0.6780 G + 0.05938 B)'
    1.8814f,                   // C_B' = (B' - Y') / 1.8814
    1.4746f                    // C_R' = (R' - Y') / 1.4746
};

struct ColorSpaceEntry
{
    QVideoFrameFormat::ColorSpace colorSpace;
    QVideoFrameFormat::ColorRange colorRange;
    ColorSpaceCoeff coefficients;
};

// clang-format off
const std::vector<ColorSpaceEntry> colorSpaces = {
    {
        QVideoFrameFormat::ColorSpace_BT709,
        QVideoFrameFormat::ColorRange_Video,
        BT709Coefficients
    },
    {
        QVideoFrameFormat::ColorSpace_BT709,
        QVideoFrameFormat::ColorRange_Full,
        BT709Coefficients
    },
    {
        QVideoFrameFormat::ColorSpace_BT2020,
        QVideoFrameFormat::ColorRange_Video,
        BT2020Coefficients
    },
    {
        QVideoFrameFormat::ColorSpace_BT2020,
        QVideoFrameFormat::ColorRange_Full,
        BT2020Coefficients
    }
};

ColorSpaceCoeff getColorSpaceCoef(QVideoFrameFormat::ColorSpace colorSpace,
                                  QVideoFrameFormat::ColorRange range)
{
    const auto it = std::find_if(colorSpaces.begin(), colorSpaces.end(),
        [&](const ColorSpaceEntry &p) {
            return p.colorSpace == colorSpace && p.colorRange == range;
        });

    if (it != colorSpaces.end())
        return it->coefficients;

    Q_ASSERT(false);

    return {};
}

QMatrix4x4 yuv2rgb(QVideoFrameFormat::ColorSpace colorSpace, QVideoFrameFormat::ColorRange range)
{
    constexpr float max8bit = static_cast<float>(255);
    constexpr float uvOffset = -128.0f/max8bit; // Really -0.5, but carried over from fixed point

    QMatrix4x4 normalizeYUV;

    if (range == QVideoFrameFormat::ColorRange_Video) {
        // YUV signal is assumed to be in limited range 8 bit representation,
        // where Y is in range [16..235] and U and V are in range [16..240].
        // Shaders use floats in [0..1], so we scale the values accordingly.
        constexpr float yRange = (235 - 16) / max8bit;
        constexpr float yOffset = -16 / max8bit;
        constexpr float uvRange = (240 - 16) / max8bit;

        // Second, stretch limited range YUV signals to full range
        normalizeYUV.scale(1/yRange, 1/uvRange, 1/uvRange);

        // First, pull limited range signals down so that they start on 0
        normalizeYUV.translate(yOffset, uvOffset, uvOffset);
    } else {
        normalizeYUV.translate(0.0f, uvOffset, uvOffset);
    }

    const auto [a, b, c, d, e] = getColorSpaceCoef(colorSpace, range);

    // Color matrix from ITU-R BT.709-6 Table 3 - Signal Format
    // Same as ITU-R BT.2020-2 Table 4 - Signal format
    const QMatrix4x4 rgb2yuv {
                a,    b,       c, 0.0f,   // Item 3.2: E_g'  = a * E_R' + b * E_G' + c * E_B'
             -a/d, -b/d, (1-c)/d, 0.0f,   // Item 3.3: E_CB' = (E_B' - E_g')/d
          (1-a)/e, -b/e,    -c/e, 0.0f,   // Item 3.3: E_CR' = (E_R' - E_g')/e
             0.0f, 0.0f,    0.0f, 1.0f
    };

    const QMatrix4x4 yuv2rgb = rgb2yuv.inverted();

    // Read backwards:
    // 1. Offset and scale YUV signal to be in range [0..1]
    // 3. Convert to RGB in range [0..1]
    return yuv2rgb * normalizeYUV;
}

// clang-format on

bool fuzzyCompareWithTolerance(const QMatrix4x4 &computed, const QMatrix4x4 &baseline,
                               float tolerance)
{
    const float *computedData = computed.data();
    const float *baselineData = baseline.data();
    for (int i = 0; i < 16; ++i) {
        const float c = computedData[i];
        const float b = baselineData[i];

        bool difference = false;
        if (qFuzzyIsNull(c) && qFuzzyIsNull(b))
            continue;

        difference = 2 * (std::abs(c - b) / (c + b)) > tolerance;

        if (difference) {
            qDebug() << "Mismatch at index" << i << c << "vs" << b;
            qDebug() << "Computed:";
            qDebug() << computed;
            qDebug() << "Baseline:";
            qDebug() << baseline;

            return false;
        }
    }
    return true;
}

bool fuzzyCompareWithTolerance(const QVector3D &computed, const QVector3D &baseline,
                               float tolerance)
{
    auto fuzzyCompare = [](float c, float b, float tolerance) {
        if (std::abs(c) < tolerance && std::abs(b) < tolerance)
            return true;

        return 2 * std::abs(c - b) / (c + b) < tolerance;
    };

    const bool equal = fuzzyCompare(computed.x(), baseline.x(), tolerance)
            && fuzzyCompare(computed.y(), baseline.y(), tolerance)
            && fuzzyCompare(computed.z(), baseline.z(), tolerance);

    if (!equal) {
        qDebug() << "Vectors are different. Computed:";
        qDebug() << computed;
        qDebug() << "Baseline:";
        qDebug() << baseline;
    }

    return equal;
}

QMatrix4x4 getColorMatrix(const QByteArray &uniformDataBytes)
{
    const auto uniformData =
            reinterpret_cast<const QVideoTextureHelper::UniformData *>(uniformDataBytes.data());
    const auto colorMatrixData = reinterpret_cast<const float *>(uniformData->colorMatrix);

    return QMatrix4x4{ colorMatrixData }.transposed();
};

class tst_qvideotexturehelper : public QObject
{
    Q_OBJECT
public:
private slots:
    void updateUniformData_populatesYUV2RGBColorMatrix_data()
    {
        QTest::addColumn<QVideoFrameFormat::ColorSpace>("colorSpace");
        QTest::addColumn<QVideoFrameFormat::ColorRange>("colorRange");

        QTest::addRow("BT709_full")
                << QVideoFrameFormat::ColorSpace_BT709 << QVideoFrameFormat::ColorRange_Full;

        QTest::addRow("BT709_video")
                << QVideoFrameFormat::ColorSpace_BT709 << QVideoFrameFormat::ColorRange_Video;

        QTest::addRow("BT2020_full")
                << QVideoFrameFormat::ColorSpace_BT2020 << QVideoFrameFormat::ColorRange_Full;

        QTest::addRow("BT2020_video")
                << QVideoFrameFormat::ColorSpace_BT2020 << QVideoFrameFormat::ColorRange_Video;
    }

    void updateUniformData_populatesYUV2RGBColorMatrix()
    {
        QFETCH(const QVideoFrameFormat::ColorSpace, colorSpace);
        QFETCH(const QVideoFrameFormat::ColorRange, colorRange);

        // Arrange
        QVideoFrameFormat format{ {}, QVideoFrameFormat::Format_NV12 };
        format.setColorSpace(colorSpace);
        format.setColorRange(colorRange);

        const QMatrix4x4 expected = yuv2rgb(colorSpace, colorRange);

        // Act
        QByteArray data;
        QVideoTextureHelper::updateUniformData(&data, format, {}, {}, 0.0);
        const QMatrix4x4 actual = getColorMatrix(data);

        // Assert
        QVERIFY(fuzzyCompareWithTolerance(actual, expected, 1e-3f));

        { // Sanity check: Color matrix maps white to white
            constexpr QVector3D expectedWhiteRgb{ 1.0f, 1.0f, 1.0f };
            const QVector3D whiteYuv = expected.inverted().map(expectedWhiteRgb);

            const QVector3D actualWhiteRgb = actual.map(whiteYuv);
            QVERIFY(fuzzyCompareWithTolerance(actualWhiteRgb, expectedWhiteRgb, 5e-4f));
        }

        { // Sanity check: Color matrix maps black to black
            constexpr QVector3D expectedBlackRgb{ 0.0f, 0.0f, 0.0f };
            const QVector3D blackYuv = expected.inverted().map(expectedBlackRgb);

            const QVector3D actualBlackRgb = actual.map(blackYuv);
            QVERIFY(fuzzyCompareWithTolerance(actualBlackRgb, expectedBlackRgb, 5e-4f));
        }
    }
};

QTEST_MAIN(tst_qvideotexturehelper)

#include "tst_qvideotexturehelper.moc"
