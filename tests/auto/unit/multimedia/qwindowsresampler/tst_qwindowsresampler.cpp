// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QtMultimedia/private/qwindowsresampler_p.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>

namespace ranges = QtMultimediaPrivate::ranges;

QAudioFormat makeAudioFormat(int sampleRate, int numberOfChannels, QAudioFormat::SampleFormat fmt)
{
    QAudioFormat ret;
    ret.setSampleFormat(fmt);
    ret.setChannelCount(numberOfChannels);
    ret.setSampleRate(sampleRate);
    ret.setChannelConfig(QAudioFormat::defaultChannelConfigForChannelCount(numberOfChannels));
    return ret;
}

template <typename Container>
auto asByteSpan(Container &c)
{
    return as_bytes(QSpan(c));
}

class tst_QWindowsResampler : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();

private slots:
    void resample_QByteArray_mono_to_stereo();
    void resample_QByteArray_stereo_to_mono();

    void resample_QByteArray_mono_to_stereo_pmr();

private:
    std::unique_ptr<QWindowsResampler> dut;
};

void tst_QWindowsResampler::initTestCase()
{
    dut = std::make_unique<QWindowsResampler>();
}

void tst_QWindowsResampler::resample_QByteArray_mono_to_stereo()
{
    auto formatMono = makeAudioFormat(44100, 1, QAudioFormat::Float);
    auto formatStereo = makeAudioFormat(44100, 2, QAudioFormat::Float);

    dut->setup(formatMono, formatStereo);

    constexpr std::array monoData{
        0.1f, 0.2f, 0.3f, 0.4f, 0.5f,
    };
    constexpr std::array stereoData{
        0.1f, 0.1f, 0.2f, 0.2f, 0.3f, 0.3f, 0.4f, 0.4f, 0.5f, 0.5f,
    };

    for (int i = 0; i != 4; ++i) {
        auto result = dut->resample(as_bytes(QSpan(monoData)));

        QCOMPARE(size_t(result.size()),
                 size_t(formatStereo.bytesForFrames(qint32(monoData.size()))));
        QCOMPARE(asByteSpan(result).size(), asByteSpan(stereoData).size());
        QVERIFY(ranges::equal(asByteSpan(result), asByteSpan(stereoData), std::equal_to<>{}));
    }
}

void tst_QWindowsResampler::resample_QByteArray_stereo_to_mono()
{
    auto formatMono = makeAudioFormat(44100, 1, QAudioFormat::Float);
    auto formatStereo = makeAudioFormat(44100, 2, QAudioFormat::Float);

    dut->setup(formatStereo, formatMono);

    constexpr std::array monoData{
        0.1f, 0.2f, 0.3f, 0.4f, 0.5f,
    };
    constexpr std::array stereoData{
        0.1f, 0.1f, 0.2f, 0.2f, 0.3f, 0.3f, 0.4f, 0.4f, 0.5f, 0.5f,
    };

    for (int i = 0; i != 4; ++i) {
        auto result = dut->resample(as_bytes(QSpan(stereoData)));

        QCOMPARE(size_t(result.size()),
                 size_t(formatMono.bytesForFrames(qint32(stereoData.size() / 2))));
        QCOMPARE(asByteSpan(result).size(), asByteSpan(monoData).size());
        QVERIFY(ranges::equal(asByteSpan(result), asByteSpan(monoData), std::equal_to<>{}));
    }
}

void tst_QWindowsResampler::resample_QByteArray_mono_to_stereo_pmr()
{
    auto formatMono = makeAudioFormat(44100, 1, QAudioFormat::Float);
    auto formatStereo = makeAudioFormat(44100, 2, QAudioFormat::Float);

    dut->setup(formatMono, formatStereo);

    constexpr std::array monoData{
        0.1f, 0.2f, 0.3f, 0.4f, 0.5f,
    };
    constexpr std::array stereoData{
        0.1f, 0.1f, 0.2f, 0.2f, 0.3f, 0.3f, 0.4f, 0.4f, 0.5f, 0.5f,
    };

    for (int i = 0; i != 4; ++i) {
        auto result = dut->resample(as_bytes(QSpan(monoData)), std::pmr::get_default_resource());

        QCOMPARE(size_t(result.size()),
                 size_t(formatStereo.bytesForFrames(qint32(monoData.size()))));
        QCOMPARE(asByteSpan(result).size(), asByteSpan(stereoData).size());
        QVERIFY(ranges::equal(asByteSpan(result), asByteSpan(stereoData), std::equal_to<>{}));
    }
}

QTEST_MAIN(tst_QWindowsResampler)

#include "tst_qwindowsresampler.moc"

