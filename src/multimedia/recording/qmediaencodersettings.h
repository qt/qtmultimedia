/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMEDIAENCODERSETTINGS_H
#define QMEDIAENCODERSETTINGS_H

#include <QtCore/qsharedpointer.h>
#include <QtCore/qstring.h>
#include <QtCore/qsize.h>
#include <QtCore/qvariant.h>
#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qmediaformat.h>

QT_BEGIN_NAMESPACE

class QMediaEncoderSettingsPrivate;
class Q_MULTIMEDIA_EXPORT QMediaEncoderSettings
{
    Q_GADGET
    Q_PROPERTY(QMediaFormat mediaFormat READ mediaFormat WRITE setMediaFormat)
    Q_PROPERTY(Quality quality READ quality WRITE setQuality)
public:
    enum Quality
    {
        VeryLowQuality,
        LowQuality,
        NormalQuality,
        HighQuality,
        VeryHighQuality
    };
    Q_ENUM(Quality)

    enum EncodingMode
    {
        ConstantQualityEncoding,
        ConstantBitRateEncoding,
        AverageBitRateEncoding,
        TwoPassEncoding
    };

    QMediaEncoderSettings();
    QMediaEncoderSettings(const QMediaFormat &format);
    QMediaEncoderSettings(const QMediaEncoderSettings& other);
    QMediaEncoderSettings& operator=(const QMediaEncoderSettings &other);
    ~QMediaEncoderSettings();

    QMediaFormat mediaFormat() const;
    void setMediaFormat(const QMediaFormat &format);

    QMediaFormat::FileFormat fileFormat() const { return mediaFormat().fileFormat(); }
    void setFileFormat(QMediaFormat::FileFormat f);

    QMediaFormat::VideoCodec videoCodec() const { return mediaFormat().videoCodec(); }
    void setVideoCodec(QMediaFormat::VideoCodec codec);

    QMediaFormat::AudioCodec audioCodec() const { return mediaFormat().audioCodec(); }
    void setAudioCodec(QMediaFormat::AudioCodec codec);

    QMimeType mimeType() const;

    EncodingMode encodingMode() const;
    void setEncodingMode(EncodingMode);

    Quality quality() const;
    void setQuality(Quality quality);

    void resolveFormat(QMediaFormat::ResolveFlags = QMediaFormat::NoFlags);

    QSize videoResolution() const;
    void setVideoResolution(const QSize &);
    void setVideoResolution(int width, int height) { setVideoResolution(QSize(width, height)); }

    qreal videoFrameRate() const;
    void setVideoFrameRate(qreal rate);

    int videoBitRate() const;
    void setVideoBitRate(int bitrate);

    int audioBitRate() const;
    void setAudioBitRate(int bitrate);

    int audioChannelCount() const;
    void setAudioChannelCount(int channels);

    int audioSampleRate() const;
    void setAudioSampleRate(int rate);

    bool operator==(const QMediaEncoderSettings &other) const;
    bool operator!=(const QMediaEncoderSettings &other) const
    { return !operator==(other); }

private:
    QSharedDataPointer<QMediaEncoderSettingsPrivate> d;
};

QT_END_NAMESPACE

#endif
