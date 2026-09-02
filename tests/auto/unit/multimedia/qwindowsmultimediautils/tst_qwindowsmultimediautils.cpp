// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtMultimedia/private/qwindowsmediafoundation_p.h>
#include <QtMultimedia/private/qwindowsmultimediautils_p.h>
#include <QtMultimedia/qcameradevice.h>
#include <QtCore/private/qcomptr_p.h>

#include <mfapi.h>

#include <optional>

namespace {

// The device populations these cases model, both dumped through IMFSourceReader.
// A Logitech Brio 500 and an MX Brio enumerate one media type per discrete rate,
// with MF_MT_FRAME_RATE and both MF_MT_FRAME_RATE_RANGE_ attributes set to that
// rate. A Lenovo Integrated Camera (USB VID_30C9 PID_0030) instead enumerates one
// YUY2 type per resolution, all at MF_MT_FRAME_RATE 30/1, and states neither
// range attribute on any of them.
constexpr QSize hd{ 1280, 720 };

ComPtr<IMFMediaType> makeVideoType(const GUID &subtype, QSize resolution)
{
    ComPtr<IMFMediaType> mediaType;
    if (FAILED(MFCreateMediaType(mediaType.GetAddressOf())))
        return {};

    mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    mediaType->SetGUID(MF_MT_SUBTYPE, subtype);
    if (resolution.isValid())
        MFSetAttributeSize(mediaType.Get(), MF_MT_FRAME_SIZE, resolution.width(),
                           resolution.height());

    return mediaType;
}

} // namespace

class tst_QWindowsMultimediaUtils : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void cameraFormatFromMediaType_usesRangeAttributes_whenPresent();
    void cameraFormatFromMediaType_fallsBackToNominalRate_withoutRanges();
    void cameraFormatFromMediaType_keepsFormat_withoutAnyRateAttribute();
    void cameraFormatFromMediaType_returnsNoFormat_withoutFrameSize();
    void cameraFormatFromMediaType_returnsNoFormat_forUnmappedSubtype();

private:
    std::optional<QMFRuntimeInit> m_mediaFoundation;
};

void tst_QWindowsMultimediaUtils::initTestCase()
{
    m_mediaFoundation.emplace();
}

void tst_QWindowsMultimediaUtils::cleanupTestCase()
{
    m_mediaFoundation.reset();
}

void tst_QWindowsMultimediaUtils::cameraFormatFromMediaType_usesRangeAttributes_whenPresent()
{
    const ComPtr<IMFMediaType> mediaType = makeVideoType(MFVideoFormat_YUY2, hd);
    QVERIFY(mediaType);
    MFSetAttributeRatio(mediaType.Get(), MF_MT_FRAME_RATE, 30, 1);
    MFSetAttributeRatio(mediaType.Get(), MF_MT_FRAME_RATE_RANGE_MIN, 5, 1);
    MFSetAttributeRatio(mediaType.Get(), MF_MT_FRAME_RATE_RANGE_MAX, 60, 1);

    const std::optional<QCameraFormat> format = QWMF::cameraFormatFromMediaType(mediaType.Get());

    QVERIFY(format);
    QCOMPARE(format->pixelFormat(), QVideoFrameFormat::Format_YUYV);
    QCOMPARE(format->resolution(), hd);
    QCOMPARE(format->minFrameRate(), 5.f);
    QCOMPARE(format->maxFrameRate(), 60.f);
}

void tst_QWindowsMultimediaUtils::cameraFormatFromMediaType_fallsBackToNominalRate_withoutRanges()
{
    const ComPtr<IMFMediaType> mediaType = makeVideoType(MFVideoFormat_YUY2, hd);
    QVERIFY(mediaType);
    MFSetAttributeRatio(mediaType.Get(), MF_MT_FRAME_RATE, 30, 1);

    const std::optional<QCameraFormat> format = QWMF::cameraFormatFromMediaType(mediaType.Get());

    QVERIFY(format);
    QCOMPARE(format->minFrameRate(), 30.f);
    QCOMPARE(format->maxFrameRate(), 30.f);
}

void tst_QWindowsMultimediaUtils::cameraFormatFromMediaType_keepsFormat_withoutAnyRateAttribute()
{
    const ComPtr<IMFMediaType> mediaType = makeVideoType(MFVideoFormat_YUY2, hd);
    QVERIFY(mediaType);

    const std::optional<QCameraFormat> format = QWMF::cameraFormatFromMediaType(mediaType.Get());

    QVERIFY(format);
    QCOMPARE(format->resolution(), hd);
    QCOMPARE(format->minFrameRate(), 0.f);
    QCOMPARE(format->maxFrameRate(), 0.f);
}

void tst_QWindowsMultimediaUtils::cameraFormatFromMediaType_returnsNoFormat_withoutFrameSize()
{
    const ComPtr<IMFMediaType> mediaType = makeVideoType(MFVideoFormat_YUY2, QSize{});
    QVERIFY(mediaType);
    MFSetAttributeRatio(mediaType.Get(), MF_MT_FRAME_RATE, 30, 1);

    QVERIFY(!QWMF::cameraFormatFromMediaType(mediaType.Get()));
}

void tst_QWindowsMultimediaUtils::cameraFormatFromMediaType_returnsNoFormat_forUnmappedSubtype()
{
    const ComPtr<IMFMediaType> mediaType = makeVideoType(MFVideoFormat_H264, hd);
    QVERIFY(mediaType);
    MFSetAttributeRatio(mediaType.Get(), MF_MT_FRAME_RATE, 30, 1);

    QVERIFY(!QWMF::cameraFormatFromMediaType(mediaType.Get()));
}

QTEST_MAIN(tst_QWindowsMultimediaUtils)

#include "tst_qwindowsmultimediautils.moc"
