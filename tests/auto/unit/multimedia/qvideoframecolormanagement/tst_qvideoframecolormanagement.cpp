// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>

#include <qvideoframe.h>
#include <qvideoframeformat.h>
#include "private/qmemoryvideobuffer_p.h"
#include <QtGui/QColorSpace>
#include <QtGui/QImage>
#include <QtCore/QPointer>

QT_USE_NAMESPACE

namespace {

struct TestParams
{
    QString fileName;
    QVideoFrameFormat::PixelFormat pixelFormat;
    QVideoFrameFormat::ColorSpace colorSpace;
    QVideoFrameFormat::ColorRange colorRange;
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

QString toString(QVideoFrameFormat::PixelFormat f)
{
    switch (f) {
    case QVideoFrameFormat::Format_NV12:
        return "nv12";
    case QVideoFrameFormat::Format_NV21:
        return "nv21";
    case QVideoFrameFormat::Format_IMC1:
        return "imc1";
    case QVideoFrameFormat::Format_IMC2:
        return "imc2";
    case QVideoFrameFormat::Format_IMC3:
        return "imc3";
    case QVideoFrameFormat::Format_IMC4:
        return "imc4";
    case QVideoFrameFormat::Format_YUV420P:
        return "420p";
    case QVideoFrameFormat::Format_YUV422P:
        return "422p";
    default:
        Q_ASSERT(false);
        return ""; // Not implemented yet
    }
}

std::vector<QVideoFrameFormat::PixelFormat> pixelFormats()
{
    return { QVideoFrameFormat::Format_NV12,    QVideoFrameFormat::Format_NV21,
             QVideoFrameFormat::Format_IMC1,    QVideoFrameFormat::Format_IMC2,
             QVideoFrameFormat::Format_IMC3,    QVideoFrameFormat::Format_IMC4,
             QVideoFrameFormat::Format_YUV420P, QVideoFrameFormat::Format_YUV422P };
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
    return QString("%1_%2_%3_%4")
            .arg(p.fileName)
            .arg(toString(p.pixelFormat))
            .arg(toString(p.colorSpace))
            .arg(toString(p.colorRange));
}

QString path(const QTemporaryDir &dir, const TestParams &param, const QString &suffix = ".png")
{
    return dir.filePath(name(param) + suffix);
}

constexpr void rgb2y(const QRgb &rgb, uchar *y)
{
    const float Y = 0.2126f * qRed(rgb) + 0.7152f * qGreen(rgb) + 0.0722f * qBlue(rgb);
    y[0] = static_cast<uchar>(std::clamp(Y + 0.5f, 0.0f, 255.0f));
}

constexpr uchar rgb2u(const QRgb &rgb)
{
    const double U = -0.0999 * qRed(rgb) + -0.3361 * qGreen(rgb) + 0.4360 * qBlue(rgb);
    return static_cast<uchar>(std::clamp(U + 127.5, 0.0, 255.0));
}

constexpr uchar rgb2v(const QRgb &rgb)
{
    const double V = 0.6150 * qRed(rgb) + -0.5586 * qGreen(rgb) + -0.0564 * qBlue(rgb);
    return static_cast<uchar>(std::clamp(V + 127.5, 0.0, 255.0));
}

void rgb2y(const QImage &image, QVideoFrame &frame, int yPlane)
{
    uchar *bits = frame.bits(yPlane);
    for (int row = 0; row < image.height(); ++row) {
        for (int col = 0; col < image.width(); ++col) {
            const QRgb pixel = image.pixel(col, row);
            rgb2y(pixel, bits + col);
        }
        bits += frame.bytesPerLine(yPlane);
    }
}

void rgb2uv(const QImage &image, QVideoFrame &frame)
{
    uchar *vBits = nullptr;
    uchar *uBits = nullptr;
    int vStride = 0;
    int uStride = 0;
    int sampleIncrement = 1;
    int verticalScale = 2;
    if (frame.pixelFormat() == QVideoFrameFormat::Format_IMC1) {
        uStride = frame.bytesPerLine(2);
        vStride = frame.bytesPerLine(1);
        uBits = frame.bits(2);
        vBits = frame.bits(1);
    } else if (frame.pixelFormat() == QVideoFrameFormat::Format_IMC2) {
        uStride = frame.bytesPerLine(1);
        vStride = frame.bytesPerLine(1);
        uBits = frame.bits(1) + vStride / 2;
        vBits = frame.bits(1);
    } else if (frame.pixelFormat() == QVideoFrameFormat::Format_IMC3) {
        uStride = frame.bytesPerLine(1);
        vStride = frame.bytesPerLine(2);
        uBits = frame.bits(1);
        vBits = frame.bits(2);
    } else if (frame.pixelFormat() == QVideoFrameFormat::Format_IMC4) {
        uStride = frame.bytesPerLine(1);
        vStride = frame.bytesPerLine(1);
        uBits = frame.bits(1);
        vBits = frame.bits(1) + vStride / 2;
    } else if (frame.pixelFormat() == QVideoFrameFormat::Format_NV12) {
        uStride = frame.bytesPerLine(1);
        vStride = frame.bytesPerLine(1);
        uBits = frame.bits(1);
        vBits = frame.bits(1) + 1;
        sampleIncrement = 2;
    } else if (frame.pixelFormat() == QVideoFrameFormat::Format_NV21) {
        uStride = frame.bytesPerLine(1);
        vStride = frame.bytesPerLine(1);
        uBits = frame.bits(1) + 1;
        vBits = frame.bits(1);
        sampleIncrement = 2;
    } else if (frame.pixelFormat() == QVideoFrameFormat::Format_YUV420P) {
        uStride = frame.bytesPerLine(1);
        vStride = frame.bytesPerLine(2);
        uBits = frame.bits(1);
        vBits = frame.bits(2);
    } else if (frame.pixelFormat() == QVideoFrameFormat::Format_YUV422P) {
        uStride = frame.bytesPerLine(1);
        vStride = frame.bytesPerLine(2);
        uBits = frame.bits(1);
        vBits = frame.bits(2);
        verticalScale = 1;
    }

    const QImage downSampled = image.scaled(image.width() / 2, image.height() / verticalScale);
    const int width = downSampled.width();
    const int height = downSampled.height();
    {
        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                const QRgb pixel = downSampled.pixel(col, row);
                uBits[col * sampleIncrement] = rgb2u(pixel);
                vBits[col * sampleIncrement] = rgb2v(pixel);
            }
            vBits += vStride;
            uBits += uStride;
        }
    }
}

void naive_rgbToYuv(const QImage &image, QVideoFrame &frame)
{
    Q_ASSERT(image.format() == QImage::Format_RGB32);
    Q_ASSERT(frame.planeCount() > 1);
    Q_ASSERT(image.size() == frame.size());

    frame.map(QVideoFrame::WriteOnly);

    rgb2y(image, frame, 0);
    rgb2uv(image, frame);

    frame.unmap();
}

QVideoFrame createTestFrame(const TestParams &params, const QImage &image)
{
    QVideoFrameFormat format(image.size(), params.pixelFormat);
    format.setColorRange(params.colorRange);
    format.setColorSpace(params.colorSpace);
    format.setColorTransfer(QVideoFrameFormat::ColorTransfer_Unknown);

    QVideoFrame frame(format);

    if (params.pixelFormat == QVideoFrameFormat::Format_IMC1
        || params.pixelFormat == QVideoFrameFormat::Format_IMC2
        || params.pixelFormat == QVideoFrameFormat::Format_IMC3
        || params.pixelFormat == QVideoFrameFormat::Format_IMC4
        || params.pixelFormat == QVideoFrameFormat::Format_NV12
        || params.pixelFormat == QVideoFrameFormat::Format_NV21
        || params.pixelFormat == QVideoFrameFormat::Format_YUV420P
        || params.pixelFormat == QVideoFrameFormat::Format_YUV422P) {
        naive_rgbToYuv(image, frame);
    } else {
        qDebug() << "Not implemented yet";
        Q_ASSERT(false);
        return {};
    }

    return frame;
}

struct ImageDiffReport
{
    int DiffCountAboveThreshold;
    int MaxDiff;
    int PixelCount;
};

double aboveThresholdDiffRatio(const ImageDiffReport &report)
{
    return static_cast<double>(report.DiffCountAboveThreshold) / report.PixelCount;
}

int maxChannelDiff(QRgb lhs, QRgb rhs)
{
    // clang-format off
    return std::max({ std::abs(qRed(lhs)   - qRed(rhs)),
                      std::abs(qGreen(lhs) - qGreen(rhs)),
                      std::abs(qBlue(lhs)  - qBlue(rhs)) });
    // clang-format on
}

std::optional<ImageDiffReport> compareImages(const QImage &computed, const QImage &baseline,
                                             int channelThreshold)
{
    const QImage lhs = computed.convertToFormat(QImage::Format_RGB32);
    const QImage rhs = baseline.convertToFormat(QImage::Format_RGB32);

    if (lhs.size() != rhs.size())
        return {};

    if (lhs.format() != rhs.format())
        return {};

    if (lhs.colorSpace() != rhs.colorSpace())
        return {};

    const QSize size = lhs.size();

    ImageDiffReport report{};
    report.PixelCount = size.width() * size.height();

    // Iterate over all pixels and update report
    for (int l = 0; l < size.height(); l++) {
        const QRgb *colorLeft = reinterpret_cast<const QRgb *>(lhs.constScanLine(l));
        const QRgb *colorRight = reinterpret_cast<const QRgb *>(rhs.constScanLine(l));
        int w = size.width();
        while (w--) {
            if ((*colorLeft++ & RGB_MASK) != (*colorRight++ & RGB_MASK)) {
                const int diff = maxChannelDiff(*colorLeft, *colorRight);
                if (diff > report.MaxDiff)
                    report.MaxDiff = diff;
                if (diff > channelThreshold) {
                    ++report.DiffCountAboveThreshold;
                }
            }
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

    QImage getReference(TestParams param) const
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

    bool saveActualImage(const TestParams &params, const QImage &image) const
    {
        if (!image.save(path(*m_testdataDir, params, "_actual.png"))) {
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

    const std::optional<ImageDiffReport> diffs =
            compareImages(actual, expected, maxChannelThreshold);
    if (!diffs)
        return diffs;

    if (diffs->MaxDiff > 0) {
        // Images are not equal, and may require manual inspection
        if (!references.saveActualImage(params, actual))
            return {};
    }

    return diffs;
}

} // namespace

class tst_qvideoframecolormanagement : public QObject
{
    Q_OBJECT
private slots:

    void toImage_savesWithCorrectColors_data()
    {
        QTest::addColumn<QString>("fileName");
        QTest::addColumn<TestParams>("params");
        for (const char *file : { "umbrellas.jpg" }) {
            for (const QVideoFrameFormat::PixelFormat pixelFormat : pixelFormats()) {
                for (const QVideoFrameFormat::ColorSpace colorSpace : colorSpaces()) {
                    for (const QVideoFrameFormat::ColorRange colorRange : colorRanges()) {
                        TestParams param{ file, pixelFormat, colorSpace, colorRange };
                        QTest::addRow("%s", name(param).toLatin1().data()) << file << param;
                    }
                }
            }
        }
    }

    void toImage_savesWithCorrectColors()
    {
        QFETCH(const QString, fileName);
        QFETCH(const TestParams, params);

        const QImage templateImage = m_reference.getTestdata(fileName);
        QVERIFY(!templateImage.isNull());

        const QVideoFrame frame = createTestFrame(params, templateImage);

        // Act
        const QImage actual = frame.toImage();

        // Assert
        constexpr int diffThreshold = 4;
        std::optional<ImageDiffReport> result =
                compareToReference(params, actual, m_reference, diffThreshold);

        QVERIFY(result);
        const double ratioAboveThreshold =
                static_cast<double>(result->DiffCountAboveThreshold) / result->PixelCount;
        QCOMPARE_LT(ratioAboveThreshold, 0.01);
        QCOMPARE_LT(result->MaxDiff, 5);
    }

private:
    ReferenceData m_reference;
};

QTEST_MAIN(tst_qvideoframecolormanagement)

#include "tst_qvideoframecolormanagement.moc"
