// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <qvideoframe.h>
#include <qvideoframeformat.h>
#include "private/qmemoryvideobuffer_p.h"
#include "private/qvideoframeconverter_p.h"
#include "private/qplatformmediaintegration_p.h"
#include "private/qimagevideobuffer_p.h"
#include "private/qvideoframe_p.h"
#include "private/qvideotexturehelper_p.h"
#include "private/qthreadlocalrhi_p.h"
#include <private/qfileutil_p.h>
#include <private/qmultimedia_enum_to_string_converter_p.h>
#include <private/osdetection_p.h>
#include <QtGui/QColorSpace>
#include <QtGui/QImage>
#include <QtCore/QPointer>
#include <QtCore/qset.h>

#include <private/mediabackendutils_p.h>

QT_BEGIN_NAMESPACE

using namespace QtMultimediaPrivate;

enum class RenderingMode { Rhi, Cpu };

enum ExcludableTextures {
    Exclude_None = 0x00,
    Exclude_R8 = 0x01,
    Exclude_RG8 = 0x02,
    Exclude_R16 = 0x04,
    Exclude_RG16 = 0x08,
    Exclude_All = Exclude_R8 | Exclude_RG8 | Exclude_R16 | Exclude_RG16
};

enum class FileType {
    Reference,
    Computed,
    Diff,
    TestName,
};

// clang-format off

QT_MM_MAKE_STRING_RESOLVER(RenderingMode, EnumName,
                           (RenderingMode::Rhi, "Rhi")
                           (RenderingMode::Cpu, "Cpu")
                          );

// clang-format on

QT_END_NAMESPACE

QT_USE_NAMESPACE
using namespace QVideoTextureHelper;

namespace {


struct TestParams
{
    QString fileName;
    QVideoFrameFormat::PixelFormat pixelFormat;
    QVideoFrameFormat::ColorSpace colorSpace;
    QVideoFrameFormat::ColorRange colorRange;
    RenderingMode renderingMode;
    ExcludableTextures excludedTextures;
};

QString toString(QVideoFrameFormat::ColorRange r)
{
    switch (r) {
    case QVideoFrameFormat::ColorRange_Video:
        return QStringLiteral("Video");
    case QVideoFrameFormat::ColorRange_Full:
        return QStringLiteral("Full");
    default:
        QTEST_ASSERT(false);
        return QString();
    }
}

std::vector<QVideoFrameFormat::ColorRange> colorRanges()
{
    return {
        QVideoFrameFormat::ColorRange_Video,
        QVideoFrameFormat::ColorRange_Full,
    };
}

const QSet s_formats{ QVideoFrameFormat::Format_ARGB8888,
                      QVideoFrameFormat::Format_ARGB8888_Premultiplied,
                      QVideoFrameFormat::Format_XRGB8888,
                      QVideoFrameFormat::Format_BGRA8888,
                      QVideoFrameFormat::Format_BGRA8888_Premultiplied,
                      QVideoFrameFormat::Format_BGRX8888,
                      QVideoFrameFormat::Format_ABGR8888,
                      QVideoFrameFormat::Format_XBGR8888,
                      QVideoFrameFormat::Format_RGBA8888,
                      QVideoFrameFormat::Format_RGBX8888,
                      QVideoFrameFormat::Format_NV12,
                      QVideoFrameFormat::Format_NV21,
                      QVideoFrameFormat::Format_IMC1,
                      QVideoFrameFormat::Format_IMC2,
                      QVideoFrameFormat::Format_IMC3,
                      QVideoFrameFormat::Format_IMC4,
                      QVideoFrameFormat::Format_AYUV,
                      QVideoFrameFormat::Format_AYUV_Premultiplied,
                      QVideoFrameFormat::Format_YV12,
                      QVideoFrameFormat::Format_YUV420P,
                      QVideoFrameFormat::Format_YUV422P,
                      QVideoFrameFormat::Format_UYVY,
                      QVideoFrameFormat::Format_YUYV,
                      QVideoFrameFormat::Format_Y8,
                      QVideoFrameFormat::Format_Y16,
                      QVideoFrameFormat::Format_P010,
                      QVideoFrameFormat::Format_P016,
                      QVideoFrameFormat::Format_YUV420P10 };

QList<QRhiTexture::Format> excludedRhiFormats(ExcludableTextures flags) {
    QList<QRhiTexture::Format> excludedFormats;
    if (flags & Exclude_R8)
        excludedFormats.append(QRhiTexture::R8);
    if (flags & Exclude_RG8)
        excludedFormats.append(QRhiTexture::RG8);
    if (flags & Exclude_R16)
        excludedFormats.append(QRhiTexture::R16);
    if (flags & Exclude_RG16)
        excludedFormats.append(QRhiTexture::RG16);
    return excludedFormats;
}

bool hasCorrespondingFFmpegFormat(QVideoFrameFormat::PixelFormat format)
{
    return format != QVideoFrameFormat::Format_AYUV
            && format != QVideoFrameFormat::Format_AYUV_Premultiplied;
}

bool supportsCpuConversion(QVideoFrameFormat::PixelFormat format)
{
    return format != QVideoFrameFormat::Format_YUV420P10;
}

QString toString(QVideoFrameFormat::PixelFormat f)
{
    return QVideoFrameFormat::pixelFormatToString(f);
}

QSet<QVideoFrameFormat::PixelFormat> pixelFormats()
{
    return s_formats;
}

QString toString(QVideoFrameFormat::ColorSpace s)
{
    switch (s) {
    case QVideoFrameFormat::ColorSpace_BT601:
        return QStringLiteral("BT601");
    case QVideoFrameFormat::ColorSpace_BT709:
        return QStringLiteral("BT709");
    case QVideoFrameFormat::ColorSpace_AdobeRgb:
        return QStringLiteral("AdobeRgb");
    case QVideoFrameFormat::ColorSpace_BT2020:
        return QStringLiteral("BT2020");
    default:
        QTEST_ASSERT(false);
        return QString();
    }
}

QString toString(ExcludableTextures excludedTextureCombination)
{
    if (excludedTextureCombination == Exclude_None) {
        return QStringLiteral("");
    }

    QStringList stringList;
    stringList.append(QStringLiteral("Exclude"));

    if (excludedTextureCombination & Exclude_R8) {
        stringList.append(QStringLiteral("R8"));
    }
    if (excludedTextureCombination & Exclude_RG8) {
        stringList.append(QStringLiteral("RG8"));
    }
    if (excludedTextureCombination & Exclude_R16) {
        stringList.append(QStringLiteral("R16"));
    }
    if (excludedTextureCombination & Exclude_RG16) {
        stringList.append(QStringLiteral("RG16"));
    }

    return stringList.join(QStringLiteral("_"));
}

std::vector<QVideoFrameFormat::ColorSpace> colorSpaces()
{
    return { QVideoFrameFormat::ColorSpace_BT601, QVideoFrameFormat::ColorSpace_BT709,
             QVideoFrameFormat::ColorSpace_AdobeRgb, QVideoFrameFormat::ColorSpace_BT2020 };
}

bool areExcludedFormatsRelevantForPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat,
                                    const QList<QRhiTexture::Format>& excludedFormats, QRhi *rhi)
{
    // Check if all excluded texture formats create a fallback chain in this pixel format
    const auto *textureDescription = QVideoTextureHelper::textureDescription(pixelFormat);
    QList<QRhiTexture::Format> encounteredFormats;

    for (int i = 0; i < textureDescription->nplanes; ++i) {
        auto rhiFormat = textureDescription->rhiTextureFormat(i, rhi);
        while (excludedFormats.contains(rhiFormat)) {
            encounteredFormats.append(rhiFormat);
            QVideoTextureHelper::setExcludedRhiTextureFormats(encounteredFormats);
            rhiFormat = textureDescription->rhiTextureFormat(i, rhi);
        }
    }

    // Reset excluded formats
    QVideoTextureHelper::setExcludedRhiTextureFormats({});

    // Return true if all the formats in excludedFormats are in encounteredFormats
    return std::all_of(excludedFormats.cbegin(), excludedFormats.cend(),
                       [&encounteredFormats](QRhiTexture::Format excludedFormat) {
        return encounteredFormats.contains(excludedFormat);
    });
}

bool areRhiTextureFormatsSupported(const QRhi& rhi, const QList<QRhiTexture::Format>& rhiFormats) {
    return std::all_of(rhiFormats.cbegin(), rhiFormats.cend(), [&rhi](QRhiTexture::Format rhiFormat) {
        return rhi.isTextureFormatSupported(rhiFormat);
    });
}

std::vector<RenderingMode> renderingModes(QVideoFrameFormat::PixelFormat pixelFormat)
{
    std::vector<RenderingMode> result;
    if (supportsCpuConversion(pixelFormat))
        result.push_back(RenderingMode::Cpu); // Only run tests on GPU if RHI is supported
    if (isRhiRenderingSupported()) {
        QRhi *rhi = qEnsureThreadLocalRhi();
        QTEST_ASSERT(rhi);
        result.push_back(RenderingMode::Rhi);
    }
    return result;
}

std::vector<ExcludableTextures>
excludableTextureCombinations(QVideoFrameFormat::PixelFormat pixelFormat)
{
    std::vector<ExcludableTextures> exclusionList;
    QRhi *rhi = qEnsureThreadLocalRhi();
    QTEST_ASSERT(rhi);

    auto addRenderingModeWithExclusions = [&](ExcludableTextures textures) {
        // We want to emulate excluding QRhi formats only if those are
        // supported by the rhi.

        auto excludedFormats = excludedRhiFormats(textures);
        if (areExcludedFormatsRelevantForPixelFormat(pixelFormat, excludedFormats, rhi)
            && areRhiTextureFormatsSupported(*rhi, excludedFormats))
            exclusionList.push_back(textures);
    };

    for (auto i = 0u; i <= static_cast<unsigned int>(Exclude_All); ++i) {
        auto flags = static_cast<ExcludableTextures>(i);

        // QTBUG-134113:
        // YUV420P10 fails on Linux CI
        const bool shouldSkip_Rhi_R16_Excluded =
                isLinux && isCI() && pixelFormat == QVideoFrameFormat::Format_YUV420P10;
        if (shouldSkip_Rhi_R16_Excluded && flags & Exclude_R16)
            continue; // Don't include any combinations that exclude R16

        addRenderingModeWithExclusions(flags);
    }

    return exclusionList;
}

QString fileName(const TestParams &p, FileType fileType)
{
    QStringList fileNameParts = { p.fileName, toString(p.pixelFormat), toString(p.colorSpace),
                                  toString(p.colorRange) };

    if (p.excludedTextures != Exclude_None && fileType != FileType::Reference)
        fileNameParts.append(toString(p.excludedTextures));

    if (p.renderingMode == RenderingMode::Cpu)
        fileNameParts.append(QStringLiteral("cpu"));

    if (fileType == FileType::Computed)
        fileNameParts.append(QStringLiteral("actual"));
    else if (fileType == FileType::Diff)
        fileNameParts.append(QStringLiteral("diff"));

    QString name = fileNameParts.join(QStringLiteral("_"))
                           .toLower()
                           .replace(QStringLiteral(" "), QStringLiteral("_"));

    if (fileType != FileType::TestName)
        name += u".png";

    return name;
}

QString resultPath(const QTemporaryDir &dir, const TestParams &params, FileType fileType)
{
    const auto renderingModeDescription = StringResolver<RenderingMode>::toQString(params.renderingMode);
    QTEST_ASSERT(renderingModeDescription);
    QDir currentDir(dir.path());
    QString resultFolderName = QStringLiteral("result_") + *renderingModeDescription;
    if (params.excludedTextures != Exclude_None)
        resultFolderName.append(QStringLiteral("_") + toString(params.excludedTextures));
    const bool subdirCreated = currentDir.exists(resultFolderName) || currentDir.mkdir(resultFolderName);
    QTEST_ASSERT(subdirCreated);

    return currentDir.filePath(resultFolderName + QDir::separator() + fileName(params, fileType));
}

QString testName(const TestParams &params)
{
    const auto renderingModeDescription =
            StringResolver<RenderingMode>::toQString(params.renderingMode);
    QTEST_ASSERT(renderingModeDescription);

    return QStringLiteral("%1, %2").arg(fileName(params, FileType::TestName), *renderingModeDescription);
}

QVideoFrame createTestFrame(const TestParams &params, const QImage &image)
{
    QVideoFrameFormat format(image.size(), params.pixelFormat);
    format.setColorRange(params.colorRange);
    format.setColorSpace(params.colorSpace);
    format.setColorTransfer(QVideoFrameFormat::ColorTransfer_Unknown);

    auto buffer = std::make_unique<QImageVideoBuffer>(image);
    QVideoFrameFormat imageFormat = {
        image.size(), QVideoFrameFormat::pixelFormatFromImageFormat(image.format())
    };

    QVideoFrame source = QVideoFramePrivate::createFrame(std::move(buffer), imageFormat);
    return QPlatformMediaIntegration::instance()->convertVideoFrame(source, format);
}

struct ImageDiffReport
{
    int DiffCountAboveThreshold; // Number of channel differences above threshold
    int MaxDiff;                 // Maximum difference between two images (max across channels)
    int PixelCount;              // Number of pixels in the image
    QImage DiffImage;            // The difference image (absolute per-channel difference)
};

int maxChannelDiff(QRgb lhs, QRgb rhs)
{
    // clang-format off
    return std::max({ std::abs(qRed(lhs)   - qRed(rhs)),
                      std::abs(qGreen(lhs) - qGreen(rhs)),
                      std::abs(qBlue(lhs)  - qBlue(rhs)) });
    // clang-format on
}

int clampedAbsDiff(int lhs, int rhs)
{
    return std::clamp(std::abs(lhs - rhs), 0, 255);
}

QRgb pixelDiff(QRgb lhs, QRgb rhs)
{
    return qRgb(clampedAbsDiff(qRed(lhs), qRed(rhs)), clampedAbsDiff(qGreen(lhs), qGreen(rhs)),
                clampedAbsDiff(qBlue(lhs), qBlue(rhs)));
}

std::optional<ImageDiffReport> compareImagesRgb32(const QImage &computed, const QImage &baseline,
                                             int channelThreshold)
{
    QTEST_ASSERT(baseline.format() == QImage::Format_RGB32);

    if (computed.size() != baseline.size())
        return {};

    if (computed.format() != baseline.format())
        return {};

    if (computed.colorSpace() != baseline.colorSpace())
        return {};

    const QSize size = baseline.size();

    ImageDiffReport report{};
    report.PixelCount = size.width() * size.height();
    report.DiffImage = QImage(size, baseline.format());

    // Iterate over all pixels and update report
    for (int l = 0; l < size.height(); l++) {
        const QRgb *colorComputed = reinterpret_cast<const QRgb *>(computed.constScanLine(l));
        const QRgb *colorBaseline = reinterpret_cast<const QRgb *>(baseline.constScanLine(l));
        QRgb *colorDiff = reinterpret_cast<QRgb *>(report.DiffImage.scanLine(l));

        int w = size.width();
        while (w--) {
            *colorDiff = pixelDiff(*colorComputed, *colorBaseline);
            if (*colorComputed != *colorBaseline) {
                const int diff = maxChannelDiff(*colorComputed, *colorBaseline);

                if (diff > report.MaxDiff)
                    report.MaxDiff = diff;

                if (diff > channelThreshold)
                    ++report.DiffCountAboveThreshold;
            }

            ++colorComputed;
            ++colorBaseline;
            ++colorDiff;
        }
    }
    return report;
}

class ReferenceData
{
public:
    ReferenceData()
    {
        m_testdataDir = QTest::qExtractTestData(QStringLiteral("testdata"));
        if (!m_testdataDir)
            m_testdataDir = QSharedPointer<QTemporaryDir>(new QTemporaryDir);
    }

    ~ReferenceData()
    {
        if (m_testdataDir->autoRemove())
            return;

        QString resultPath = m_testdataDir->path();
        if (qEnvironmentVariableIsSet("COIN_CTEST_RESULTSDIR")) {
            const QDir sourceDir = m_testdataDir->path();
            const QDir resultsDir{ qEnvironmentVariable("COIN_CTEST_RESULTSDIR") };
            if (!copyAllFiles(sourceDir, resultsDir)) {
                qDebug() << "Failed to copy files to COIN_CTEST_RESULTSDIR";
            } else {
                resultPath = resultsDir.path();
            }
        }

        qDebug() << "Images with differences were found. The output images with differences"
                 << "can be found in" << resultPath << ". Review the images and if the"
                 << "differences are expected, please update the testdata with the new"
                 << "output images";
    }

    QImage getReference(const TestParams &param) const
    {
        const QString referenceName = fileName(param, FileType::Reference);
        const QString referencePath = m_testdataDir->filePath(referenceName);
        QImage result;
        if (result.load(referencePath))
            return result;
        return {};
    }

    void saveNewReference(const QImage &reference, const TestParams &params) const
    {
        const QString filename = resultPath(*m_testdataDir, params, FileType::Reference);
        if (!reference.save(filename)) {
            qDebug() << "Failed to save reference file";
            QTEST_ASSERT(false);
        }

        m_testdataDir->setAutoRemove(false);
    }

    bool saveComputedImage(const TestParams &params, const QImage &image, FileType fileType) const
    {
        if (!image.save(resultPath(*m_testdataDir, params, fileType))) {
            qDebug() << "Unexpectedly failed to save actual image to file";
            QTEST_ASSERT(false);
            return false;
        }
        m_testdataDir->setAutoRemove(false);
        return true;
    }

    QImage getTestdata(const QString &name)
    {
        const QString filePath = m_testdataDir->filePath(name);
        QImage image;
        if (image.load(filePath))
            return image;
        return {};
    }

private:
    QSharedPointer<QTemporaryDir> m_testdataDir;
};

std::optional<ImageDiffReport> compareToReference(const TestParams &params, const QImage &actual,
                                                  const ReferenceData &references,
                                                  int maxChannelThreshold)
{
    const QImage expected = references.getReference(params);
    if (expected.isNull()) {
        // Reference image does not exist. Create one. Adding this to
        // testdata directory is a manual job.
        references.saveNewReference(actual, params);
        qDebug() << "Reference image is missing. Please update testdata directory with the missing "
                    "reference image";
        return {};
    }

    // Convert to RGB32 to simplify image comparison
    const QImage computed = actual.convertToFormat(QImage::Format_RGB32);
    const QImage baseline = expected.convertToFormat(QImage::Format_RGB32);

    std::optional<ImageDiffReport> diffReport = compareImagesRgb32(computed, baseline, maxChannelThreshold);
    if (!diffReport)
        return {};

    if (diffReport->MaxDiff > 0) {
        // Images are not equal, and may require manual inspection
        if (!references.saveComputedImage(params, computed, FileType::Computed))
            return {};
        if (!references.saveComputedImage(params, diffReport->DiffImage, FileType::Diff))
            return {};
    }

    return diffReport;
}

} // namespace

class tst_qvideoframecolormanagement : public QObject
{
    Q_OBJECT
private slots:
    void cleanup() { QVideoTextureHelper::setExcludedRhiTextureFormats({}); }

    void initTestCase()
    {
        QSKIP_IF_NOT_FFMPEG("This test requires the FFmpeg backend to create test frames");
    }

    void qImageFromVideoFrame_returnsQImageWithCorrectColors_data()
    {
        QTest::addColumn<QString>("fileName");
        QTest::addColumn<TestParams>("params");
        for (const char *file : { "umbrellas.jpg" }) {
            for (const QVideoFrameFormat::PixelFormat pixelFormat : pixelFormats()) {
                if (!hasCorrespondingFFmpegFormat(pixelFormat))
                    continue;

                for (const QVideoFrameFormat::ColorSpace colorSpace : colorSpaces()) {
                    for (const QVideoFrameFormat::ColorRange colorRange : colorRanges()) {
                        for (const RenderingMode renderingMode : renderingModes(pixelFormat)) {
                            if (renderingMode == RenderingMode::Cpu) {
                                TestParams param{ QString::fromLatin1(file),
                                                  pixelFormat,
                                                  colorSpace,
                                                  colorRange,
                                                  renderingMode,
                                                  Exclude_None };
                                QTest::newRow(testName(param).toLatin1().data()) << file << param;
                            } else {
                                for (const ExcludableTextures excludableTextureCombination :
                                     excludableTextureCombinations(pixelFormat)) {
                                    TestParams param{
                                        QString::fromLatin1(file),
                                        pixelFormat,
                                        colorSpace,
                                        colorRange,
                                        renderingMode,
                                        excludableTextureCombination
                                    };
                                    QTest::newRow(testName(param).toLatin1().data()) << file << param;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // This test is a regression test for the QMultimedia display pipeline.
    // It compares rendered output (as created by qImageFromVideoFrame)
    // against reference images stored to file. The reference images were
    // created by the test itself, and does not verify correctness, just
    // changes to render output.
    void qImageFromVideoFrame_returnsQImageWithCorrectColors()
    {
        QFETCH(const QString, fileName);
        QFETCH(const TestParams, params);

        // Skip fallback from R16->RGBA8 because of QTBUG-126277
        QRhi *rhi = qEnsureThreadLocalRhi();
        if (params.pixelFormat == QVideoFrameFormat::Format_YUV420P10
            && (params.excludedTextures & Exclude_R16
                || (rhi && !rhi->isTextureFormatSupported(QRhiTexture::R16)))
            && (params.excludedTextures & Exclude_RG8
                || (rhi && !rhi->isTextureFormatSupported(QRhiTexture::RG8))))
            QSKIP("Fallback from R16->RGBA8 skipped due to QTBUG-126277");

        // Arrange
        applyExcludedTextures(params.excludedTextures);
        const QImage templateImage = m_reference.getTestdata(fileName);
        QVERIFY(!templateImage.isNull());

        const QVideoFrame frame = createTestFrame(params, templateImage);

        // Act
        const QImage actual =
                qImageFromVideoFrame(frame, params.renderingMode == RenderingMode::Cpu);

        // Assert
        constexpr int diffThreshold = 4;
        std::optional<ImageDiffReport> result =
                compareToReference(params, actual, m_reference, diffThreshold);

        // Sanity checks
        QVERIFY(result.has_value());
        QCOMPARE_GT(result->PixelCount, 0);

        // Verify that images are similar
        const double ratioAboveThreshold =
                static_cast<double>(result->DiffCountAboveThreshold) / result->PixelCount;

        // These thresholds are empirically determined to allow tests to pass in CI.
        // If tests fail, review the difference between the reference and actual
        // output to determine if it is a platform dependent inaccuracy before
        // adjusting the limits
        QCOMPARE_LT(ratioAboveThreshold, 0.01); // Fraction of pixels with larger differences
        QCOMPARE_LT(result->MaxDiff, 6); // Maximum per-channel difference
    }

private:
    void applyExcludedTextures(ExcludableTextures excludedTextures)
    {
        QVideoTextureHelper::setExcludedRhiTextureFormats(excludedRhiFormats(excludedTextures));
    }

private:
    ReferenceData m_reference;
};

QTEST_MAIN(tst_qvideoframecolormanagement)

#include "tst_qvideoframecolormanagement.moc"
