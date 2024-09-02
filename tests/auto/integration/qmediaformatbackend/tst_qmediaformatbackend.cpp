// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtCore/QSysInfo>
#include <QtMultimedia/qmediaformat.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>

QT_USE_NAMESPACE

using namespace Qt::StringLiterals;

QString toString(QMediaFormat::ConversionMode mode)
{
    switch (mode) {
    case QMediaFormat::Encode:
        return QStringLiteral("encode");
    case QMediaFormat::Decode:
        return QStringLiteral("decode");
    }
    Q_ASSERT(false);
    return {};
}

class tst_qmediaformatbackend : public QObject
{
    Q_OBJECT

private slots:
    void print_formatSupport_noVerify()
    {

        qWarning() << "Dumping codec and format support for" << QSysInfo::prettyProductName()
                   << "with" << QPlatformMediaIntegration::instance()->name() << "media backend";

        for (const auto &mode : { QMediaFormat::Encode, QMediaFormat::Decode }) {
            QMediaFormat mediaFormat;
            qWarning() << "Supported file formats for" << toString(mode);
            for (const auto &format : mediaFormat.supportedFileFormats(mode)) {
                qWarning() << "    " << QMediaFormat::fileFormatName(format);
            }

            qWarning() << "Supported audio codecs for" << toString(mode);
            for (const auto &format : mediaFormat.supportedAudioCodecs(mode)) {
                qWarning() << "    " << QMediaFormat::audioCodecName(format) << ":"
                           << QMediaFormat::audioCodecDescription(format);
            }

            qWarning() << "Supported video codecs for" << toString(mode);
            for (const auto &format : mediaFormat.supportedVideoCodecs(mode)) {
                qWarning() << "    " << QMediaFormat::videoCodecName(format) << ":"
                           << QMediaFormat::videoCodecDescription(format);
            }
        }
    }
};

QTEST_GUILESS_MAIN(tst_qmediaformatbackend)
#include "tst_qmediaformatbackend.moc"
