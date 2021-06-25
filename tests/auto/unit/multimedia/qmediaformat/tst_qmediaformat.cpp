/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QDebug>
#include <QtMultimedia/qmediaformat.h>

class tst_QMediaFormat : public QObject
{
    Q_OBJECT

private slots:
    void testResolveForEncoding();
};

void tst_QMediaFormat::testResolveForEncoding()
{
    QMediaFormat format;

    auto hasVideoCodecs = !format.supportedVideoCodecs(QMediaFormat::Encode).isEmpty();
    bool hasWav = format.supportedFileFormats(QMediaFormat::Encode).contains(QMediaFormat::Wave);

    // Resolve codecs for audio only stream
    format.resolveForEncoding(QMediaFormat::NoFlags);
    QVERIFY(format.audioCodec() != QMediaFormat::AudioCodec::Unspecified);
    QVERIFY(format.fileFormat() != QMediaFormat::FileFormat::UnspecifiedFormat);
    QVERIFY(format.videoCodec() == QMediaFormat::VideoCodec::Unspecified);

    // Resolve codecs for audio/video stream
    format.resolveForEncoding(QMediaFormat::RequiresVideo);
    QVERIFY(format.audioCodec() != QMediaFormat::AudioCodec::Unspecified);
    QVERIFY(format.fileFormat() != QMediaFormat::FileFormat::UnspecifiedFormat);
    if (hasVideoCodecs)
        QVERIFY(format.videoCodec() != QMediaFormat::VideoCodec::Unspecified);
    else
        QVERIFY(format.videoCodec() == QMediaFormat::VideoCodec::Unspecified);

    // Resolve again for audio only stream
    format.resolveForEncoding(QMediaFormat::NoFlags);
    QVERIFY(format.videoCodec() == QMediaFormat::VideoCodec::Unspecified);

    // check some specific conditions
    if (hasWav) {
        QMediaFormat f(QMediaFormat::Mpeg4Audio);
        if (!f.supportedAudioCodecs(QMediaFormat::Encode).contains(QMediaFormat::AudioCodec::Wave)) {
            qDebug() << "testing!";
            format.setFileFormat(QMediaFormat::Mpeg4Audio);
            format.setAudioCodec(QMediaFormat::AudioCodec::Wave);
            format.resolveForEncoding(QMediaFormat::NoFlags);
            QVERIFY(format.fileFormat() == QMediaFormat::Mpeg4Audio);
            QVERIFY(format.audioCodec() != QMediaFormat::AudioCodec::Wave);

            format = {};
            format.setFileFormat(QMediaFormat::Wave);
            format.setAudioCodec(QMediaFormat::AudioCodec::AAC);
            format.resolveForEncoding(QMediaFormat::NoFlags);
            QVERIFY(format.fileFormat() == QMediaFormat::Wave);
            QVERIFY(format.audioCodec() == QMediaFormat::AudioCodec::Wave);
        }
    }

}

QTEST_MAIN(tst_QMediaFormat)
#include "tst_qmediaformat.moc"
