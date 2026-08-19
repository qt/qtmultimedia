// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../../../../../../../src/plugins/multimedia/windows/common/qwindowsasfpicture_p.h"

#include <QtTest/qtest.h>

#include <QtCore/qstring.h>

#include <array>
#include <vector>

QT_USE_NAMESPACE

namespace {

// A minimal valid 1x1 RGBA PNG (verified via IHDR width/height at offsets 16-23).
constexpr std::array<BYTE, 70> kPngBytes = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
    0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
    0x89, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x44, 0x41,
    0x54, 0x78, 0xDA, 0x63, 0xF8, 0xCF, 0xC0, 0xF0,
    0x1F, 0x00, 0x05, 0x00, 0x01, 0xFF, 0x56, 0xC7,
    0x2F, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
    0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
};

// Appends raw little-endian UTF-16 code units, without a terminator (matches how
// imageFromAsfFlatPicture() walks the buffer two bytes at a time).
void appendUtf16CodeUnits(std::vector<BYTE> &buffer, const QString &s)
{
    for (QChar ch : s) {
        const ushort code = ch.unicode();
        buffer.push_back(static_cast<BYTE>(code & 0xFF));
        buffer.push_back(static_cast<BYTE>((code >> 8) & 0xFF));
    }
}

void appendUtf16String(std::vector<BYTE> &buffer, const QString &s)
{
    appendUtf16CodeUnits(buffer, s);
    buffer.push_back(0);
    buffer.push_back(0);
}

// bPictureType is never inspected by the parser under test, so its value is arbitrary.
// The parser uses the packed QMM_ASF_FLAT_PICTURE (sizeof 5, see qwindowsasfpicture_p.h),
// so the header is built from that type rather than the SDK's ASF_FLAT_PICTURE, whose size
// depends on compiler padding.
void appendHeader(std::vector<BYTE> &buffer, DWORD dwDataLen)
{
    QMM_ASF_FLAT_PICTURE header;
    header.bPictureType = 3;
    header.dwDataLen = dwDataLen;

    const auto *raw = reinterpret_cast<const BYTE *>(&header);
    buffer.insert(buffer.end(), raw, raw + sizeof(QMM_ASF_FLAT_PICTURE));
}

BLOB makeBlob(std::vector<BYTE> &storage)
{
    BLOB blob;
    blob.cbSize = static_cast<ULONG>(storage.size());
    blob.pBlobData = storage.empty() ? nullptr : storage.data();
    return blob;
}

} // namespace

class tst_QWindowsAsfPicture : public QObject
{
    Q_OBJECT

private slots:
    void validPicture_decodesSuccessfully();
    void blobSmallerThanHeader_returnsNullImage();
    void missingMimeTerminator_returnsNullImage();
    void dwDataLenExceedsAvailableBytes_returnsNullImage();
    void truncatedImageBytes_returnsNullImage();
};

void tst_QWindowsAsfPicture::validPicture_decodesSuccessfully()
{
    std::vector<BYTE> buffer;
    appendHeader(buffer, static_cast<DWORD>(kPngBytes.size()));
    appendUtf16String(buffer, QStringLiteral("image/png"));
    appendUtf16String(buffer, QString()); // empty description
    buffer.insert(buffer.end(), kPngBytes.begin(), kPngBytes.end());

    const BLOB blob = makeBlob(buffer);
    const QImage image = imageFromAsfFlatPicture(blob);

    QVERIFY(!image.isNull());
    QCOMPARE(image.size(), QSize(1, 1));
}

void tst_QWindowsAsfPicture::blobSmallerThanHeader_returnsNullImage()
{
    // The size check is "<=", so this exact boundary value must still be rejected.
    std::vector<BYTE> buffer(sizeof(QMM_ASF_FLAT_PICTURE), 0);

    const BLOB blob = makeBlob(buffer);
    const QImage image = imageFromAsfFlatPicture(blob);
    QVERIFY(image.isNull());
}

void tst_QWindowsAsfPicture::missingMimeTerminator_returnsNullImage()
{
    // No NUL terminator anywhere before the buffer ends. The skip loop's "p + 1 < end" guard
    // stops before reading p[0]/p[1] out of bounds; the subsequent "p += 2" then overshoots
    // to end + 1, caught by "if (p > end) return {}" before dwDataLen/data are touched.
    std::vector<BYTE> buffer;
    appendHeader(buffer, 0);
    appendUtf16CodeUnits(buffer, QStringLiteral("image/png"));
    // Deliberately no terminator appended.

    const BLOB blob = makeBlob(buffer);
    const QImage image = imageFromAsfFlatPicture(blob);
    QVERIFY(image.isNull());
}

void tst_QWindowsAsfPicture::dwDataLenExceedsAvailableBytes_returnsNullImage()
{
    // dwDataLen claims far more bytes than actually follow.
    std::vector<BYTE> buffer;
    appendHeader(buffer, static_cast<DWORD>(kPngBytes.size()) + 1000);
    appendUtf16String(buffer, QStringLiteral("image/png"));
    appendUtf16String(buffer, QString());
    buffer.insert(buffer.end(), kPngBytes.begin(), kPngBytes.end());

    const BLOB blob = makeBlob(buffer);
    const QImage image = imageFromAsfFlatPicture(blob);
    QVERIFY(image.isNull());
}

void tst_QWindowsAsfPicture::truncatedImageBytes_returnsNullImage()
{
    // dwDataLen matches the remaining byte count (bounds checks pass), but the bytes aren't
    // a decodable image.
    std::vector<BYTE> buffer;
    const std::vector<BYTE> garbage = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    appendHeader(buffer, static_cast<DWORD>(garbage.size()));
    appendUtf16String(buffer, QStringLiteral("image/png"));
    appendUtf16String(buffer, QString());
    buffer.insert(buffer.end(), garbage.begin(), garbage.end());

    const BLOB blob = makeBlob(buffer);
    const QImage image = imageFromAsfFlatPicture(blob);
    QVERIFY(image.isNull());
}

QTEST_MAIN(tst_QWindowsAsfPicture)
#include "tst_qwindowsasfpicture.moc"
