/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTMEDIANAMESPACE_H
#define QTMEDIANAMESPACE_H

#include <QtCore/qpair.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qstring.h>

#include <qtmultimediadefs.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Multimedia)

#define Q_DECLARE_METADATA(key) Q_MULTIMEDIA_EXPORT extern const QString key

namespace QtMultimedia
{
    namespace MetaData {
        // Common
        Q_DECLARE_METADATA(Title);
        Q_DECLARE_METADATA(SubTitle);
        Q_DECLARE_METADATA(Author);
        Q_DECLARE_METADATA(Comment);
        Q_DECLARE_METADATA(Description);
        Q_DECLARE_METADATA(Category);
        Q_DECLARE_METADATA(Genre);
        Q_DECLARE_METADATA(Year);
        Q_DECLARE_METADATA(Date);
        Q_DECLARE_METADATA(UserRating);
        Q_DECLARE_METADATA(Keywords);
        Q_DECLARE_METADATA(Language);
        Q_DECLARE_METADATA(Publisher);
        Q_DECLARE_METADATA(Copyright);
        Q_DECLARE_METADATA(ParentalRating);
        Q_DECLARE_METADATA(RatingOrganization);

        // Media
        Q_DECLARE_METADATA(Size);
        Q_DECLARE_METADATA(MediaType);
        Q_DECLARE_METADATA(Duration);

        // Audio
        Q_DECLARE_METADATA(AudioBitRate);
        Q_DECLARE_METADATA(AudioCodec);
        Q_DECLARE_METADATA(AverageLevel);
        Q_DECLARE_METADATA(ChannelCount);
        Q_DECLARE_METADATA(PeakValue);
        Q_DECLARE_METADATA(SampleRate);

        // Music
        Q_DECLARE_METADATA(AlbumTitle);
        Q_DECLARE_METADATA(AlbumArtist);
        Q_DECLARE_METADATA(ContributingArtist);
        Q_DECLARE_METADATA(Composer);
        Q_DECLARE_METADATA(Conductor);
        Q_DECLARE_METADATA(Lyrics);
        Q_DECLARE_METADATA(Mood);
        Q_DECLARE_METADATA(TrackNumber);
        Q_DECLARE_METADATA(TrackCount);

        Q_DECLARE_METADATA(CoverArtUrlSmall);
        Q_DECLARE_METADATA(CoverArtUrlLarge);

        // Image/Video
        Q_DECLARE_METADATA(Resolution);
        Q_DECLARE_METADATA(PixelAspectRatio);

        // Video
        Q_DECLARE_METADATA(VideoFrameRate);
        Q_DECLARE_METADATA(VideoBitRate);
        Q_DECLARE_METADATA(VideoCodec);

        Q_DECLARE_METADATA(PosterUrl);

        // Movie
        Q_DECLARE_METADATA(ChapterNumber);
        Q_DECLARE_METADATA(Director);
        Q_DECLARE_METADATA(LeadPerformer);
        Q_DECLARE_METADATA(Writer);

        // Photos
        Q_DECLARE_METADATA(CameraManufacturer);
        Q_DECLARE_METADATA(CameraModel);
        Q_DECLARE_METADATA(Event);
        Q_DECLARE_METADATA(Subject);
        Q_DECLARE_METADATA(Orientation);
        Q_DECLARE_METADATA(ExposureTime);
        Q_DECLARE_METADATA(FNumber);
        Q_DECLARE_METADATA(ExposureProgram);
        Q_DECLARE_METADATA(ISOSpeedRatings);
        Q_DECLARE_METADATA(ExposureBiasValue);
        Q_DECLARE_METADATA(DateTimeOriginal);
        Q_DECLARE_METADATA(DateTimeDigitized);
        Q_DECLARE_METADATA(SubjectDistance);
        Q_DECLARE_METADATA(MeteringMode);
        Q_DECLARE_METADATA(LightSource);
        Q_DECLARE_METADATA(Flash);
        Q_DECLARE_METADATA(FocalLength);
        Q_DECLARE_METADATA(ExposureMode);
        Q_DECLARE_METADATA(WhiteBalance);
        Q_DECLARE_METADATA(DigitalZoomRatio);
        Q_DECLARE_METADATA(FocalLengthIn35mmFilm);
        Q_DECLARE_METADATA(SceneCaptureType);
        Q_DECLARE_METADATA(GainControl);
        Q_DECLARE_METADATA(Contrast);
        Q_DECLARE_METADATA(Saturation);
        Q_DECLARE_METADATA(Sharpness);
        Q_DECLARE_METADATA(DeviceSettingDescription);

        Q_DECLARE_METADATA(PosterImage);
        Q_DECLARE_METADATA(CoverArtImage);
        Q_DECLARE_METADATA(ThumbnailImage);
    }

    enum SupportEstimate
    {
        NotSupported,
        MaybeSupported,
        ProbablySupported,
        PreferredService
    };

    enum EncodingQuality
    {
        VeryLowQuality,
        LowQuality,
        NormalQuality,
        HighQuality,
        VeryHighQuality
    };

    enum EncodingMode
    {
        ConstantQualityEncoding,
        ConstantBitRateEncoding,
        AverageBitRateEncoding,
        TwoPassEncoding
    };

    enum AvailabilityError
    {
        NoError,
        ServiceMissingError,
        BusyError,
        ResourceError
    };

}

#undef Q_DECLARE_METADATA

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QtMultimedia::AvailabilityError)
Q_DECLARE_METATYPE(QtMultimedia::SupportEstimate)
Q_DECLARE_METATYPE(QtMultimedia::EncodingMode)
Q_DECLARE_METATYPE(QtMultimedia::EncodingQuality)

QT_END_HEADER


#endif
