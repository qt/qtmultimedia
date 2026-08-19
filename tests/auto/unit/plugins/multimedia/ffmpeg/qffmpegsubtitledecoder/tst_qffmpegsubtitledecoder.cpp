// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <qobject.h>

#include <QtFFmpegMediaPluginImpl/private/qffmpegstreamdecoder_p.h>

#include <cstring>

using namespace QFFmpeg;

namespace {

AVSubtitleRect makeTextRect(std::vector<char> &storage, const char *content)
{
    storage.assign(content, content + std::strlen(content) + 1); // include NUL
    AVSubtitleRect rect{};
    rect.text = storage.data();
    return rect;
}

AVSubtitleRect makeAssRect(std::vector<char> &storage, const char *content)
{
    storage.assign(content, content + std::strlen(content) + 1); // include NUL
    AVSubtitleRect rect{};
    rect.ass = storage.data();
    return rect;
}

} // namespace

class tst_QFFmpegSubtitleDecoder : public QObject
{
    Q_OBJECT

private slots:
    void textField_plainUtf8_returnedVerbatim();
    void assField_skipsFirstEightCommaFields();
    void assField_fewerThanEightCommas_returnsEmptyString();
    void escapeSequences_convertedToRealNewlines();
    void crlf_convertedToRealNewline();
    void multipleRects_joinedWithNewline();
    void trailingNewline_isChopped();
};

void tst_QFFmpegSubtitleDecoder::textField_plainUtf8_returnedVerbatim()
{
    // Arrange
    std::vector<char> storage;
    AVSubtitleRect rect = makeTextRect(storage, "Hello World");
    AVSubtitleRect *rects[1] = { &rect };

    AVSubtitle subtitle;
    memset(&subtitle, 0, sizeof(subtitle));
    subtitle.num_rects = 1;
    subtitle.rects = rects;

    // Act
    const QString actual = subtitleTextFromAVSubtitle(subtitle);

    // Assert
    QCOMPARE(actual, QStringLiteral("Hello World"));
}

void tst_QFFmpegSubtitleDecoder::assField_skipsFirstEightCommaFields()
{
    // Arrange
    std::vector<char> storage;
    AVSubtitleRect rect = makeAssRect(storage, "1,2,3,4,5,6,7,8,Dialogue text");
    AVSubtitleRect *rects[1] = { &rect };

    AVSubtitle subtitle;
    memset(&subtitle, 0, sizeof(subtitle));
    subtitle.num_rects = 1;
    subtitle.rects = rects;

    // Act
    const QString actual = subtitleTextFromAVSubtitle(subtitle);

    // Assert
    // The byte-walk stops right after the 8th comma, so the 8 numeric fields and their
    // separating commas are skipped.
    QCOMPARE(actual, QStringLiteral("Dialogue text"));
}

void tst_QFFmpegSubtitleDecoder::assField_fewerThanEightCommas_returnsEmptyString()
{
    // Arrange
    std::vector<char> storage;
    AVSubtitleRect rect = makeAssRect(storage, "a,b,c");
    AVSubtitleRect *rects[1] = { &rect };

    AVSubtitle subtitle;
    memset(&subtitle, 0, sizeof(subtitle));
    subtitle.num_rects = 1;
    subtitle.rects = rects;

    // Act
    const QString actual = subtitleTextFromAVSubtitle(subtitle);

    // Assert
    // With fewer than 8 commas, the byte-walk runs off the end of the string instead of
    // breaking out, so the "remainder" is empty rather than the whole/last field.
    QCOMPARE(actual, QString());
}

void tst_QFFmpegSubtitleDecoder::escapeSequences_convertedToRealNewlines()
{
    // Arrange
    // "\\N"/"\\n" here are the literal 2-character sequences backslash+N/n, not real newlines.
    std::vector<char> storage;
    AVSubtitleRect rect = makeTextRect(storage, "Line1\\NLine2\\nLine3");
    AVSubtitleRect *rects[1] = { &rect };

    AVSubtitle subtitle;
    memset(&subtitle, 0, sizeof(subtitle));
    subtitle.num_rects = 1;
    subtitle.rects = rects;

    // Act
    const QString actual = subtitleTextFromAVSubtitle(subtitle);

    // Assert
    QCOMPARE(actual, QStringLiteral("Line1\nLine2\nLine3"));
}

void tst_QFFmpegSubtitleDecoder::crlf_convertedToRealNewline()
{
    // Arrange
    std::vector<char> storage;
    AVSubtitleRect rect = makeTextRect(storage, "Line1\r\nLine2");
    AVSubtitleRect *rects[1] = { &rect };

    AVSubtitle subtitle;
    memset(&subtitle, 0, sizeof(subtitle));
    subtitle.num_rects = 1;
    subtitle.rects = rects;

    // Act
    const QString actual = subtitleTextFromAVSubtitle(subtitle);

    // Assert
    QCOMPARE(actual, QStringLiteral("Line1\nLine2"));
}

void tst_QFFmpegSubtitleDecoder::multipleRects_joinedWithNewline()
{
    // Arrange
    std::vector<char> storage0;
    std::vector<char> storage1;
    AVSubtitleRect rect0 = makeTextRect(storage0, "First");
    AVSubtitleRect rect1 = makeTextRect(storage1, "Second");
    AVSubtitleRect *rects[2] = { &rect0, &rect1 };

    AVSubtitle subtitle;
    memset(&subtitle, 0, sizeof(subtitle));
    subtitle.num_rects = 2;
    subtitle.rects = rects;

    // Act
    const QString actual = subtitleTextFromAVSubtitle(subtitle);

    // Assert
    QCOMPARE(actual, QStringLiteral("First\nSecond"));
}

void tst_QFFmpegSubtitleDecoder::trailingNewline_isChopped()
{
    // Arrange
    // The literal "\\n" escape is normalized to a real newline first, then chopped.
    std::vector<char> storage;
    AVSubtitleRect rect = makeTextRect(storage, "Only line\\n");
    AVSubtitleRect *rects[1] = { &rect };

    AVSubtitle subtitle;
    memset(&subtitle, 0, sizeof(subtitle));
    subtitle.num_rects = 1;
    subtitle.rects = rects;

    // Act
    const QString actual = subtitleTextFromAVSubtitle(subtitle);

    // Assert
    QCOMPARE(actual, QStringLiteral("Only line"));
}

QTEST_MAIN(tst_QFFmpegSubtitleDecoder)

#include "tst_qffmpegsubtitledecoder.moc"
