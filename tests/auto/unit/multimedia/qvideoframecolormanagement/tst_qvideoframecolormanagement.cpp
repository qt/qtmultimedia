// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <qvideoframe.h>
#include <qvideoframeformat.h>
#include "private/qmemoryvideobuffer_p.h"
#include "private/qvideoframeconverter_p.h"
#include "private/qplatformmediaintegration_p.h"
#include "private/qimagevideobuffer_p.h"
#include <QtGui/QColorSpace>
#include <QtGui/QImage>
#include <QtCore/QPointer>

#include "../../../integration/shared/mediabackendutils.h"

QT_USE_NAMESPACE

namespace {

struct TestParams
{
    QString fileName;
    QVideoFrameFormat::PixelFormat pixelFormat;
    QVideoFrameFormat::ColorSpace colorSpace;
    QVideoFrameFormat::ColorRange colorRange;
    bool forceCpu;
};

QString toString(QVideoFrameFormat::ColorRange r)
{
    switch (r) {
    case QVideoFrameFormat::ColorRange_Video:
        return "Video";
    case QVideoFrameFormat::ColorRange_Full:
        return "Full";
    default:
        Q_ASSERT(false);
        return "";
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

bool isSupportedPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat)
{
#ifdef Q_OS_ANDROID
    // TODO: QTBUG-125238
    switch (pixelFormat) {
    case QVideoFrameFormat::Format_Y16:
    case QVideoFrameFormat::Format_P010:
    case QVideoFrameFormat::Format_P016:
    case QVideoFrameFormat::Format_YUV420P10:
        return false;
    default:
        return true;
    }
#else
    return true;
#endif
}


QString toString(QVideoFrameFormat::ColorSpace s)
{
    switch (s) {
    case QVideoFrameFormat::ColorSpace_BT601:
        return "BT601";
    case QVideoFrameFormat::ColorSpace_BT709:
        return "BT709";
    case QVideoFrameFormat::ColorSpace_AdobeRgb:
        return "AdobeRgb";
    case QVideoFrameFormat::ColorSpace_BT2020:
        return "BT2020";
    default:
        Q_ASSERT(false);
        return "";
    }
}

std::vector<QVideoFrameFormat::ColorSpace> colorSpaces()
{
    return { QVideoFrameFormat::ColorSpace_BT601, QVideoFrameFormat::ColorSpace_BT709,
             QVideoFrameFormat::ColorSpace_AdobeRgb, QVideoFrameFormat::ColorSpace_BT2020 };
}

QString name(const TestParams &p)
{
    QString name = QStringLiteral("%1_%2_%3_%4%5")
                                 .arg(p.fileName)
                                 .arg(toString(p.pixelFormat))
                                 .arg(toString(p.colorSpace))
                                 .arg(toString(p.colorRange))
                                 .arg(p.forceCpu ? "_cpu" : "")
                                 .toLower();
    name.replace(" ", "_");
    return name;
}

QString path(const QTemporaryDir &dir, const TestParams &param, const QString &suffix = ".png")
{
    return dir.filePath(name(param) + suffix);
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

    QVideoFrame source{ buffer.release(), imageFormat };
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
    Q_ASSERT(baseline.format() == QImage::Format_RGB32);

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

bool copyAllFiles(const QDir &source, const QDir &dest)
{
    if (!source.exists() || !dest.exists())
        return false;

    QDirIterator it(source);
    while (it.hasNext()) {
        QFileInfo file{ it.next() };
        if (file.isFile()) {
            const QString destination = dest.absolutePath() + "/" + file.fileName();
            QFile::copy(file.absoluteFilePath(), destination);
        }
    }

    return true;
}

class ReferenceData
{
public:
    ReferenceData()
    {
        m_testdataDir = QTest::qExtractTestData("testdata");
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
        const QString referenceName = name(param);
        const QString referencePath = m_testdataDir->filePath(referenceName + ".png");
        QImage result;
        if (result.load(referencePath))
            return result;
        return {};
    }

    void saveNewReference(const QImage &reference, const TestParams &params) const
    {
        const QString filename = path(*m_testdataDir, params);
        if (!reference.save(filename)) {
            qDebug() << "Failed to save reference file";
            Q_ASSERT(false);
        }

        m_testdataDir->setAutoRemove(false);
    }

    bool saveComputedImage(const TestParams &params, const QImage &image, const QString& suffix) const
    {
        if (!image.save(path(*m_testdataDir, params, suffix))) {
            qDebug() << "Unexpectedly failed to save actual image to file";
            Q_ASSERT(false);
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
        if (!references.saveComputedImage(params, computed, "_actual.png"))
            return {};
        if (!references.saveComputedImage(params, diffReport->DiffImage, "_diff.png"))
            return {};
    }

    return diffReport;
}

} // namespace

class tst_qvideoframecolormanagement : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        if (!isFFMPEGPlatform())
            QSKIP("This test requires the ffmpeg backend to create test frames");
    }

    void qImageFromVideoFrame_returnsQImageWithCorrectColors_data()
    {
        QTest::addColumn<QString>("fileName");
        QTest::addColumn<TestParams>("params");
        for (const char *file : { "umbrellas.jpg" }) {
            for (const QVideoFrameFormat::PixelFormat pixelFormat : pixelFormats()) {
                for (const QVideoFrameFormat::ColorSpace colorSpace : colorSpaces()) {
                    for (const QVideoFrameFormat::ColorRange colorRange : colorRanges()) {

                        QList<bool> cpuChoices = { true };
                        if (isRhiRenderingSupported())
                            cpuChoices.push_back(false); // Only run tests on GPU if RHI is supported

                        for (const bool forceCpu : cpuChoices) {

                            if (!isSupportedPixelFormat(pixelFormat))
                                continue;

                            if (forceCpu && !supportsCpuConversion(pixelFormat))
                                continue; // TODO: CPU Conversion not implemented

                            if (!hasCorrespondingFFmpegFormat(pixelFormat))
                                continue;

                            TestParams param{
                                file, pixelFormat, colorSpace, colorRange, forceCpu,
                            };
                            QTest::addRow("%s", name(param).toLatin1().data()) << file << param;
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

        const QImage templateImage = m_reference.getTestdata(fileName);
        QVERIFY(!templateImage.isNull());

        const QVideoFrame frame = createTestFrame(params, templateImage);

        // Act
        const QImage actual =
                qImageFromVideoFrame(frame, QtVideo::Rotation::None, false, false, params.forceCpu);

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
    ReferenceData m_reference;
};

QTEST_MAIN(tst_qvideoframecolormanagement)

#include "tst_qvideoframecolormanagement.moc"
