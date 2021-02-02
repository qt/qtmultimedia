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
#include <QtMultimedia/qmultimedia.h>
#include <QtMultimedia/qmediaformat.h>

QT_BEGIN_NAMESPACE

class QMediaEncoderSettingsPrivate;
class Q_MULTIMEDIA_EXPORT QMediaEncoderSettings : public QMediaFormat
{
public:
    QMediaEncoderSettings();
    QMediaEncoderSettings(FileFormat format);
    QMediaEncoderSettings(const QMediaEncoderSettings& other);
    QMediaEncoderSettings& operator=(const QMediaEncoderSettings &other);
    ~QMediaEncoderSettings();

    QMultimedia::EncodingMode encodingMode() const;
    void setEncodingMode(QMultimedia::EncodingMode);

    QMultimedia::EncodingQuality quality() const;
    void setQuality(QMultimedia::EncodingQuality quality);

    enum ResolveMode {
        AudioOnly,
        AudioAndVideo
    };

    void resolveFormat(ResolveMode mode = AudioAndVideo);

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

private:
    QSharedDataPointer<QMediaEncoderSettingsPrivate> d;
};

class QImageEncoderSettingsPrivate;
class Q_MULTIMEDIA_EXPORT QImageEncoderSettings
{
public:
    QImageEncoderSettings();
    QImageEncoderSettings(const QImageEncoderSettings& other);

    ~QImageEncoderSettings();

    QImageEncoderSettings& operator=(const QImageEncoderSettings &other);
    bool operator==(const QImageEncoderSettings &other) const;
    bool operator!=(const QImageEncoderSettings &other) const;

    bool isNull() const;

    QString codec() const;
    void setCodec(const QString &);

    QSize resolution() const;
    void setResolution(const QSize &);
    void setResolution(int width, int height);

    QMultimedia::EncodingQuality quality() const;
    void setQuality(QMultimedia::EncodingQuality quality);

    QVariant encodingOption(const QString &option) const;
    QVariantMap encodingOptions() const;
    void setEncodingOption(const QString &option, const QVariant &value);
    void setEncodingOptions(const QVariantMap &options);

private:
    QSharedDataPointer<QImageEncoderSettingsPrivate> d;
};

QT_END_NAMESPACE

#endif
