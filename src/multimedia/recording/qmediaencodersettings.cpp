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

#include "qmediaencodersettings.h"
#include "qmediaformat.h"
#include <private/qplatformmediaintegration_p.h>
#include <private/qplatformmediaformatinfo_p.h>
#include <qmimetype.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMediaEncoderSettings

    \brief The QMediaEncoderSettings class provides a settings to encode a media file.

    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_recording

    A media encoder settings object is used to specify the encoder
    settings used by QMediaRecorder. Settings are selected by
    constructing a QMediaEncoderSettings object specifying an output file format,
    setting the desired properties and then passing it to a QMediaRecorder instance
    using the QMediaRecorder::setEncoderSettings() function.

    \snippet multimedia-snippets/media.cpp Audio encoder settings

    \sa QMediaRecorder
*/

class QMediaEncoderSettingsPrivate  : public QSharedData
{
public:
    QMediaEncoderSettings::EncodingMode encodingMode = QMediaEncoderSettings::ConstantQualityEncoding;
    QMediaEncoderSettings::Quality quality = QMediaEncoderSettings::NormalQuality;

    QMediaFormat format;
    int audioBitrate = -1;
    int audioSampleRate = -1;
    int audioChannels = -1;

    QSize videoResolution = QSize(-1, -1);
    int videoFrameRate = -1;
    int videoBitRate = -1;
};

/*!
    Creates andefault media encoder settings object. Qt will automatically try to
    pick the best possible encoding for the content.
*/
QMediaEncoderSettings::QMediaEncoderSettings()
    : QMediaEncoderSettings(QMediaFormat::UnspecifiedFormat)
{
}


/*!
    Creates an audio encoder settings object with a given \a format.
*/
QMediaEncoderSettings::QMediaEncoderSettings(const QMediaFormat &format)
    : d(new QMediaEncoderSettingsPrivate)
{
    d->format = format;
}

QMediaFormat QMediaEncoderSettings::mediaFormat() const
{
    return d->format;
}

void QMediaEncoderSettings::setMediaFormat(const QMediaFormat &format)
{
    d->format = format;
}

void QMediaEncoderSettings::setFileFormat(QMediaFormat::FileFormat f)
{
    d->format.setFileFormat(f);
}

void QMediaEncoderSettings::setVideoCodec(QMediaFormat::VideoCodec codec)
{
    d->format.setVideoCodec(codec);
}

void QMediaEncoderSettings::setAudioCodec(QMediaFormat::AudioCodec codec)
{
    d->format.setAudioCodec(codec);
}

QMimeType QMediaEncoderSettings::mimeType() const
{
    return d->format.mimeType();
}

/*!
    Creates a copy of the encoder settings object \a other.
*/
QMediaEncoderSettings::QMediaEncoderSettings(const QMediaEncoderSettings &other) = default;

QMediaEncoderSettings &QMediaEncoderSettings::operator=(const QMediaEncoderSettings &other) = default;

QMediaEncoderSettings::~QMediaEncoderSettings() = default;


/*!
    \enum QMediaEncoderSettings::EncodingQuality

    Enumerates quality encoding levels.

    \value VeryLowQuality
    \value LowQuality
    \value NormalQuality
    \value HighQuality
    \value VeryHighQuality
*/

/*!
    \enum QMediaEncoderSettings::EncodingMode

    Enumerates encoding modes.

    \value ConstantQualityEncoding Encoding will aim to have a constant quality, adjusting bitrate to fit.
    \value ConstantBitRateEncoding Encoding will use a constant bit rate, adjust quality to fit.
    \value AverageBitRateEncoding Encoding will try to keep an average bitrate setting, but will use
           more or less as needed.
    \value TwoPassEncoding The media will first be processed to determine the characteristics,
           and then processed a second time allocating more bits to the areas
           that need it.
*/

/*!
    Returns the encoding mode.

    \sa EncodingMode
*/
QMediaEncoderSettings::EncodingMode QMediaEncoderSettings::encodingMode() const
{
    return d->encodingMode;
}

/*!
    Sets the encoding \a mode setting.

    If ConstantQualityEncoding is set, the quality
    encoding parameter is used and bit rates are ignored,
    otherwise the bitrates are used.

    \sa encodingMode(), EncodingMode
*/
void QMediaEncoderSettings::setEncodingMode(EncodingMode mode)
{
    d->encodingMode = mode;
}

QMediaEncoderSettings::Quality QMediaEncoderSettings::quality() const
{
    return d->quality;
}

void QMediaEncoderSettings::setQuality(Quality quality)
{
    d->quality = quality;
}

/*!
    Resolves the format to a format that is supported by QMediaRecorder.

    This method tries to find the best possible match for unspecified settings.
    Settings that are not supported by the encoder will be modified to the closest
    match that is supported.
 */
void QMediaEncoderSettings::resolveFormat(QMediaFormat::ResolveFlags flags)
{
    d->format.resolveForEncoding(flags);
}

/*!
    Returns the resolution of the encoded video.
*/
QSize QMediaEncoderSettings::videoResolution() const
{
    return d->videoResolution;
}

/*!
    Sets the \a resolution of the encoded video.

    An empty QSize indicates the encoder should make an optimal choice based on
    what is available from the video source and the limitations of the codec.
*/
void QMediaEncoderSettings::setVideoResolution(const QSize &size)
{
    d->videoResolution = size;
}

/*! \fn void QMediaEncoderSettings::setVideoResolution(int width, int height)

    Sets the \a width and \a height of the resolution of the encoded video.

    \overload
*/

/*!
    Returns the video frame rate.
*/
qreal QMediaEncoderSettings::videoFrameRate() const
{
    return d->videoFrameRate;
}

/*!
    \fn QVideoEncoderSettings::setFrameRate(qreal rate)

    Sets the video frame \a rate.

    A value of 0 indicates the encoder should make an optimal choice based on what is available
    from the video source and the limitations of the codec.
*/
void QMediaEncoderSettings::setVideoFrameRate(qreal rate)
{
    d->videoFrameRate = rate;
}

/*!
    Returns the bit rate of the compressed video stream in bits per second.
*/
int QMediaEncoderSettings::videoBitRate() const
{
    return d->videoBitRate;
}

/*!
    Sets the video bit \a rate in bits per second.
*/
void QMediaEncoderSettings::setVideoBitRate(int bitrate)
{
    d->videoBitRate = bitrate;
}

/*!
    Returns the bit rate of the compressed audio stream in bits per second.
*/
int QMediaEncoderSettings::audioBitRate() const
{
    return d->audioBitrate;
}

/*!
    Sets the audio bit \a rate in bits per second.
*/
void QMediaEncoderSettings::setAudioBitRate(int bitrate)
{
    d->audioBitrate = bitrate;
}

/*!
    Returns the number of audio channels.
*/
int QMediaEncoderSettings::audioChannelCount() const
{
    return d->audioChannels;
}

/*!
    Sets the number of audio \a channels.

    A value of -1 indicates the encoder should make an optimal choice based on
    what is available from the audio source and the limitations of the codec.
*/
void QMediaEncoderSettings::setAudioChannelCount(int channels)
{
    d->audioChannels = channels;
}

/*!
    Returns the audio sample rate in Hz.
*/
int QMediaEncoderSettings::audioSampleRate() const
{
    return d->audioSampleRate;
}

/*!
    Sets the audio sample \a rate in Hz.

    A value of -1 indicates the encoder should make an optimal choice based on what is avaialbe
    from the audio source and the limitations of the codec.
*/
void QMediaEncoderSettings::setAudioSampleRate(int rate)
{
    d->audioSampleRate = rate;
}

bool QMediaEncoderSettings::operator==(const QMediaEncoderSettings &other) const
{
    if (d == other.d)
        return true;
    return d->format == other.d->format &&
        d->encodingMode == other.d->encodingMode &&
        d->quality == other.d->quality &&
        d->audioBitrate == other.d->audioBitrate &&
        d->audioSampleRate == other.d->audioSampleRate &&
        d->audioChannels == other.d->audioChannels &&
        d->videoResolution == other.d->videoResolution &&
        d->videoFrameRate == other.d->videoFrameRate &&
        d->videoBitRate == other.d->videoBitRate;
}

QT_END_NAMESPACE
