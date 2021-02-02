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
    bool isNull = true;
    QMultimedia::EncodingMode encodingMode = QMultimedia::ConstantQualityEncoding;
    QMultimedia::EncodingQuality quality = QMultimedia::NormalQuality;

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
QMediaEncoderSettings::QMediaEncoderSettings(FileFormat format)
    : QMediaFormat(format),
      d(new QMediaEncoderSettingsPrivate)
{

}

/*!
    Creates a copy of the encoder settings object \a other.
*/
QMediaEncoderSettings::QMediaEncoderSettings(const QMediaEncoderSettings &other)
    : QMediaFormat(other),
      d(other.d)
{
}

QMediaEncoderSettings &QMediaEncoderSettings::operator=(const QMediaEncoderSettings &other)
{
    QMediaFormat::operator=(other);
    d = other.d;
    return *this;
}

QMediaEncoderSettings::~QMediaEncoderSettings()
{
}

/*!
    Returns the encoding mode.

    \sa QMultimedia::EncodingMode
*/
QMultimedia::EncodingMode QMediaEncoderSettings::encodingMode() const
{
    return d->encodingMode;
}

/*!
    Sets the encoding \a mode setting.

    If QMultimedia::ConstantQualityEncoding is set, the quality
    encoding parameter is used and bit rates are ignored,
    otherwise the bitrates are used.

    \sa encodingMode(), QMultimedia::EncodingMode
*/
void QMediaEncoderSettings::setEncodingMode(QMultimedia::EncodingMode mode)
{
    d->encodingMode = mode;
}

QMultimedia::EncodingQuality QMediaEncoderSettings::quality() const
{
    return d->quality;
}

void QMediaEncoderSettings::setQuality(QMultimedia::EncodingQuality quality)
{
    d->quality = quality;
}

/*!
    Resolves the format to a format that is supported by QMediaRecorder.

    This method tries to find the best possible match for unspecified settings.
    Settings that are not supported by the encoder will be modified to the closest
    match that is supported.
 */
void QMediaEncoderSettings::resolveFormat(QMediaEncoderSettings::ResolveMode mode)
{
    QMediaFormatPrivate::resolveForEncoding(this, mode == AudioOnly);
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


class QAudioEncoderSettingsPrivate  : public QSharedData
{
public:
    bool isNull = true;
    QMultimedia::EncodingMode encodingMode = QMultimedia::ConstantQualityEncoding;
    QString codec;
    int bitrate = -1;
    int sampleRate = -1;
    int channels = -1;
    QMultimedia::EncodingQuality quality = QMultimedia::NormalQuality;
    QVariantMap encodingOptions;
};


class QImageEncoderSettingsPrivate  : public QSharedData
{
public:
    bool isNull = true;
    QString codec;
    QSize resolution;
    QMultimedia::EncodingQuality quality = QMultimedia::NormalQuality;
    QVariantMap encodingOptions;
};

/*!
    \class QImageEncoderSettings


    \brief The QImageEncoderSettings class provides a set of image encoder
    settings.

    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_camera

    A image encoder settings object is used to specify the image encoder
    settings used by QCameraImageCapture.  Image encoder settings are selected
    by constructing a QImageEncoderSettings object, setting the desired
    properties and then passing it to a QCameraImageCapture instance using the
    QCameraImageCapture::setImageSettings() function.

    \snippet multimedia-snippets/media.cpp Image encoder settings

    \sa QImageEncoderControl
*/

/*!
    Constructs a null image encoder settings object.
*/

QImageEncoderSettings::QImageEncoderSettings()
    :d(new QImageEncoderSettingsPrivate)
{
}

/*!
    Constructs a copy of the image encoder settings object \a other.
*/

QImageEncoderSettings::QImageEncoderSettings(const QImageEncoderSettings& other)
    :d(other.d)
{
}

/*!
    Destroys a image encoder settings object.
*/

QImageEncoderSettings::~QImageEncoderSettings()
{
}

/*!
    Assigns the value of \a other to a image encoder settings object.
*/
QImageEncoderSettings &QImageEncoderSettings::operator=(const QImageEncoderSettings &other)
{
    d = other.d;
    return *this;
}

/*!
    Determines if \a other is of equal value to a image encoder settings
    object.

    Returns true if the settings objects are of equal value, and false if they
    are not of equal value.
*/
bool QImageEncoderSettings::operator==(const QImageEncoderSettings &other) const
{
    return (d == other.d) ||
           (d->isNull == other.d->isNull &&
            d->quality == other.d->quality &&
            d->codec == other.d->codec &&
            d->resolution == other.d->resolution &&
            d->encodingOptions == other.d->encodingOptions);

}

/*!
    Determines if \a other is of equal value to a image encoder settings
    object.

    Returns true if the settings objects are not of equal value, and false if
    they are of equal value.
*/
bool QImageEncoderSettings::operator!=(const QImageEncoderSettings &other) const
{
    return !(*this == other);
}

/*!
    Identifies if a image encoder settings object is uninitalized.

    Returns true if the settings are null, and false if they are not.
*/
bool QImageEncoderSettings::isNull() const
{
    return d->isNull;
}

/*!
    Returns the image codec.
*/

QString QImageEncoderSettings::codec() const
{
    return d->codec;
}

/*!
    Sets the image \a codec.
*/
void QImageEncoderSettings::setCodec(const QString& codec)
{
    d->isNull = false;
    d->codec = codec;
}

/*!
    Returns the resolution of the encoded image.
*/

QSize QImageEncoderSettings::resolution() const
{
    return d->resolution;
}

/*!
    Sets the \a resolution of the encoded image.

    An empty QSize indicates the encoder should make an optimal choice based on
    what is available from the image source and the limitations of the codec.
*/

void QImageEncoderSettings::setResolution(const QSize &resolution)
{
    d->isNull = false;
    d->resolution = resolution;
}

/*!
    Sets the \a width and \a height of the resolution of the encoded image.

    \overload
*/

void QImageEncoderSettings::setResolution(int width, int height)
{
    d->isNull = false;
    d->resolution = QSize(width, height);
}

/*!
    Returns the image encoding quality.
*/

QMultimedia::EncodingQuality QImageEncoderSettings::quality() const
{
    return d->quality;
}

/*!
    Sets the image encoding \a quality.
*/

void QImageEncoderSettings::setQuality(QMultimedia::EncodingQuality quality)
{
    d->isNull = false;
    d->quality = quality;
}

/*!
    Returns the value of encoding \a option.

    \sa setEncodingOption(), encodingOptions()
*/
QVariant QImageEncoderSettings::encodingOption(const QString &option) const
{
    return d->encodingOptions.value(option);
}

/*!
    Returns the all the encoding options as QVariantMap.

    \sa encodingOption(), setEncodingOptions()
*/
QVariantMap QImageEncoderSettings::encodingOptions() const
{
    return d->encodingOptions;
}

/*!
    Set the encoding \a option \a value.

    The supported set and meaning of encoding options are
    system and selected codec specific.

    \sa encodingOption(), setEncodingOptions()
*/
void QImageEncoderSettings::setEncodingOption(const QString &option, const QVariant &value)
{
    d->isNull = false;
    if (value.isNull())
        d->encodingOptions.remove(option);
    else
        d->encodingOptions.insert(option, value);
}

/*!
    Replace all the encoding options with \a options.

    The supported set and meaning of encoding options are
    system and selected codec specific.

    \sa encodingOption(), setEncodingOption()
*/
void QImageEncoderSettings::setEncodingOptions(const QVariantMap &options)
{
    d->isNull = false;
    d->encodingOptions = options;
}


QT_END_NAMESPACE
