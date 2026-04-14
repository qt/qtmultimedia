// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "avfmetadata_p.h"
#include <qdarwinformatsinfo_p.h>
#include <avfmediaplayer_p.h>

#include <QtCore/qbuffer.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qlocale.h>
#include <QtCore/qurl.h>
#include <QtCore/private/qcore_mac_p.h>
#include <QtCore/qsemaphore.h>
#include <QImage>
#include <QtMultimedia/qvideoframe.h>

#if __has_include(<AppKit/AppKit.h>)
#import <AppKit/AppKit.h>
#endif

#import <CoreFoundation/CoreFoundation.h>

QT_USE_NAMESPACE

using namespace std::chrono_literals;

struct AVMetadataIDs {
    AVMetadataIdentifier common;
    AVMetadataIdentifier iTunes;
    AVMetadataIdentifier quickTime;
    AVMetadataIdentifier ID3;
    AVMetadataIdentifier quickTimeUserData;
    AVMetadataIdentifier isoUserData;
};

const AVMetadataIDs keyToAVMetaDataID[] = {
    // Title
    { AVMetadataCommonIdentifierTitle, AVMetadataIdentifieriTunesMetadataSongName,
      AVMetadataIdentifierQuickTimeMetadataTitle,
      AVMetadataIdentifierID3MetadataTitleDescription,
      nil, AVMetadata3GPUserDataKeyTitle },
    // Author
    { AVMetadataCommonIdentifierAuthor,AVMetadataIdentifieriTunesMetadataAuthor,
      AVMetadataIdentifierQuickTimeMetadataAuthor, nil,
      AVMetadataQuickTimeUserDataKeyAuthor, AVMetadata3GPUserDataKeyAuthor },
    // Comment
    { nil, AVMetadataIdentifieriTunesMetadataUserComment,
      AVMetadataIdentifierQuickTimeMetadataComment, AVMetadataIdentifierID3MetadataComments,
      AVMetadataQuickTimeUserDataKeyComment, nil },
    // Description
    { AVMetadataCommonIdentifierDescription,AVMetadataIdentifieriTunesMetadataDescription,
      AVMetadataIdentifierQuickTimeMetadataDescription, nil,
      AVMetadataQuickTimeUserDataKeyDescription, AVMetadata3GPUserDataKeyDescription },
    // Genre
    { nil, AVMetadataIdentifieriTunesMetadataUserGenre,
      AVMetadataIdentifierQuickTimeMetadataGenre, nil,
      AVMetadataQuickTimeUserDataKeyGenre, AVMetadata3GPUserDataKeyGenre },
    // Date
    { AVMetadataCommonIdentifierCreationDate, AVMetadataIdentifieriTunesMetadataReleaseDate,
      AVMetadataIdentifierQuickTimeMetadataCreationDate, AVMetadataIdentifierID3MetadataDate,
      AVMetadataQuickTimeUserDataKeyCreationDate, AVMetadataISOUserDataKeyDate },
    // Language
    { AVMetadataCommonIdentifierLanguage, nil, nil, AVMetadataIdentifierID3MetadataLanguage, nil, nil },
    // Publisher
    { AVMetadataCommonIdentifierPublisher, AVMetadataIdentifieriTunesMetadataPublisher,
      AVMetadataIdentifierQuickTimeMetadataPublisher, AVMetadataIdentifierID3MetadataPublisher, nil, nil },
    // Copyright
    { AVMetadataCommonIdentifierCopyrights, AVMetadataIdentifieriTunesMetadataCopyright,
      AVMetadataIdentifierQuickTimeMetadataCopyright, AVMetadataIdentifierID3MetadataCopyright,
      AVMetadataQuickTimeUserDataKeyCopyright, AVMetadataISOUserDataKeyCopyright },
    // Url
    { nil, nil, nil, AVMetadataIdentifierID3MetadataOfficialAudioSourceWebpage, nil, nil },
    // Duration
    { nil, nil, nil, AVMetadataIdentifierID3MetadataLength, nil, nil },
    // MediaType
    { AVMetadataCommonIdentifierType, nil, nil, AVMetadataIdentifierID3MetadataContentType, nil, nil },
    // FileFormat
    { nil, nil, nil, AVMetadataIdentifierID3MetadataFileType, nil, nil },
    // AudioBitRate
    { nil, nil, nil, nil, nil, nil },
    // AudioCodec
    { nil, nil, nil, nil, nil, nil },
    // VideoBitRate
    { nil, nil, nil, nil, nil, nil },
    // VideoCodec
    { nil, nil, nil, nil, nil, nil },
    // VideoFrameRate
    { nil, nil, AVMetadataIdentifierQuickTimeMetadataCameraFrameReadoutTime, nil, nil, nil },
    // AlbumTitle
    { AVMetadataCommonIdentifierAlbumName, AVMetadataIdentifieriTunesMetadataAlbum,
      AVMetadataIdentifierQuickTimeMetadataAlbum, AVMetadataIdentifierID3MetadataAlbumTitle,
      AVMetadataQuickTimeUserDataKeyAlbum, AVMetadata3GPUserDataKeyAlbumAndTrack },
    // AlbumArtist
    { nil, AVMetadataIdentifieriTunesMetadataAlbumArtist, nil, nil,
      AVMetadataQuickTimeUserDataKeyArtist, AVMetadata3GPUserDataKeyPerformer },
    // ContributingArtist
    { AVMetadataCommonIdentifierArtist, AVMetadataIdentifieriTunesMetadataArtist,
      AVMetadataIdentifierQuickTimeMetadataArtist, nil, nil, nil },
    // TrackNumber
    { nil, AVMetadataIdentifieriTunesMetadataTrackNumber,
      nil, AVMetadataIdentifierID3MetadataTrackNumber, nil, nil },
    // Composer
    { nil, AVMetadataIdentifieriTunesMetadataComposer,
      AVMetadataIdentifierQuickTimeMetadataComposer, AVMetadataIdentifierID3MetadataComposer, nil, nil },
    // LeadPerformer
    { nil, AVMetadataIdentifieriTunesMetadataPerformer,
      AVMetadataIdentifierQuickTimeMetadataPerformer, AVMetadataIdentifierID3MetadataLeadPerformer, nil, nil },
    // ThumbnailImage
    { nil, nil, nil, AVMetadataIdentifierID3MetadataAttachedPicture, nil, nil },
    // CoverArtImage
    { AVMetadataCommonIdentifierArtwork, AVMetadataIdentifieriTunesMetadataCoverArt,
      AVMetadataIdentifierQuickTimeMetadataArtwork, nil, nil, nil },
    // Orientation
    { nil, nil, AVMetadataIdentifierQuickTimeMetadataVideoOrientation, nil, nil, nil },
    // Resolution
    { nil, nil, nil, nil, nil, nil },
    // HasHdrContent
    { nil, nil, nil, nil, nil, nil }
};

static AVMetadataIdentifier toIdentifier(QMediaMetaData::Key key, AVMetadataKeySpace keySpace)
{
    static_assert(sizeof(keyToAVMetaDataID) / sizeof(AVMetadataIDs) == QMediaMetaData::NumMetaData);

    AVMetadataIdentifier identifier = nil;
    if ([keySpace isEqualToString:AVMetadataKeySpaceiTunes]) {
        identifier = keyToAVMetaDataID[key].iTunes;
    } else if ([keySpace isEqualToString:AVMetadataKeySpaceID3]) {
        identifier = keyToAVMetaDataID[key].ID3;
    } else if ([keySpace isEqualToString:AVMetadataKeySpaceQuickTimeMetadata]) {
        identifier = keyToAVMetaDataID[key].quickTime;
    } else  {
        identifier = keyToAVMetaDataID[key].common;
    }
    return identifier;
}

static std::optional<QMediaMetaData::Key> toKey(AVMetadataItem *item)
{
    static_assert(sizeof(keyToAVMetaDataID) / sizeof(AVMetadataIDs) == QMediaMetaData::NumMetaData);

    // The item identifier may be different than the ones we support,
    // so check by common key first, as it will get the metadata
    // irrespective of the format.
    AVMetadataKey commonKey = item.commonKey;
    if (commonKey.length != 0) {
        if ([commonKey isEqualToString:AVMetadataCommonKeyTitle]) {
            return QMediaMetaData::Title;
        } else if ([commonKey isEqualToString:AVMetadataCommonKeyDescription]) {
            return QMediaMetaData::Description;
        } else if ([commonKey isEqualToString:AVMetadataCommonKeyPublisher]) {
            return QMediaMetaData::Publisher;
        } else if ([commonKey isEqualToString:AVMetadataCommonKeyCreationDate]) {
            return QMediaMetaData::Date;
        } else if ([commonKey isEqualToString:AVMetadataCommonKeyType]) {
            return QMediaMetaData::MediaType;
        } else if ([commonKey isEqualToString:AVMetadataCommonKeyLanguage]) {
            return QMediaMetaData::Language;
        } else if ([commonKey isEqualToString:AVMetadataCommonKeyCopyrights]) {
            return QMediaMetaData::Copyright;
        } else if ([commonKey isEqualToString:AVMetadataCommonKeyAlbumName]) {
            return QMediaMetaData::AlbumTitle;
        } else if ([commonKey isEqualToString:AVMetadataCommonKeyAuthor]) {
            return QMediaMetaData::Author;
        } else if ([commonKey isEqualToString:AVMetadataCommonKeyArtist]) {
            return QMediaMetaData::ContributingArtist;
        } else if ([commonKey isEqualToString:AVMetadataCommonKeyArtwork]) {
            return QMediaMetaData::CoverArtImage;
        }
    }

    // Check by identifier if no common key found
    // No need to check for the common keySpace since there's no common key
    enum keySpaces { iTunes, QuickTime, QuickTimeUserData, IsoUserData, ID3, Other } itemKeySpace;
    itemKeySpace = Other;
    AVMetadataKeySpace keySpace = [item keySpace];
    AVMetadataIdentifier identifier = [item identifier];

    if ([keySpace isEqualToString:AVMetadataKeySpaceiTunes]) {
        itemKeySpace = iTunes;
    } else if ([keySpace isEqualToString:AVMetadataKeySpaceQuickTimeMetadata]) {
        itemKeySpace = QuickTime;
    } else if ([keySpace isEqualToString:AVMetadataKeySpaceQuickTimeUserData]) {
        itemKeySpace = QuickTimeUserData;
    } else if ([keySpace isEqualToString:AVMetadataKeySpaceISOUserData]) {
        itemKeySpace = IsoUserData;
    } else if (([keySpace isEqualToString:AVMetadataKeySpaceID3])) {
        itemKeySpace = ID3;
    }

    for (int key = 0; key < QMediaMetaData::NumMetaData; key++) {
        AVMetadataIdentifier idForKey = nil;
        switch (itemKeySpace) {
        case iTunes:
            idForKey = keyToAVMetaDataID[key].iTunes;
            break;
        case QuickTime:
            idForKey = keyToAVMetaDataID[key].quickTime;
            break;
        case ID3:
            idForKey = keyToAVMetaDataID[key].ID3;
            break;
        case QuickTimeUserData:
            idForKey = keyToAVMetaDataID[key].quickTimeUserData;
            break;
        case IsoUserData:
            idForKey = keyToAVMetaDataID[key].isoUserData;
            break;
        default:
            continue;
        }

        if ([identifier isEqualToString:idForKey])
            return QMediaMetaData::Key(key);
    }

    return std::nullopt;
}

static QMediaMetaData fromAVMetadata(NSArray *metadataItems)
{
    QMediaMetaData metadata;

    for (AVMetadataItem* item in metadataItems) {
        auto key = toKey(item);
        if (!key)
            continue;

        // Handle artwork (binary image data)
        if (*key == QMediaMetaData::ThumbnailImage || *key == QMediaMetaData::CoverArtImage) {
            NSData *data = [item dataValue];
            if (data) {
                QImage image;
                image.loadFromData(QByteArray::fromNSData(data));
                if (!image.isNull())
                    metadata.insert(*key, image);
            }
            continue;
        }

        // Handle dates — prefer dateValue over stringValue, as some
        // items (e.g. creation time from MP4 mvhd) have no stringValue
        if (*key == QMediaMetaData::Date) {
            NSDate *dateValue = [item dateValue];
            if (dateValue) {
                QDateTime dt = QDateTime::fromNSDate(dateValue);
                if (dt.isValid()) {
                    metadata.insert(*key, dt);
                    continue;
                }
            }
            // Fall through to try stringValue as ISO 8601
            const QString str = QString::fromNSString([item stringValue]);
            if (!str.isNull()) {
                QDateTime dt = QDateTime::fromString(str, Qt::ISODate);
                if (dt.isValid()) {
                    metadata.insert(*key, dt);
                    continue;
                }
                metadata.insert(*key, str);
            }
            continue;
        }

        const QString value = QString::fromNSString([item stringValue]);
        if (!value.isNull())
            metadata.insert(*key, value);
    }
    return metadata;
}

QMediaMetaData AVFMetaData::fromAsset(AVAsset *asset)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    QMediaMetaData metadata = fromAVMetadata([asset metadata]);

    // On macOS 15 and below, [asset metadata] returns an empty array for certain
    // formats (e.g. MP3 with ID3 tags), while [asset commonMetadata] still provides
    // the data. Merge commonMetadata to fill any gaps.
    {
        QMediaMetaData common = fromAVMetadata([asset commonMetadata]);
        for (auto key : common.keys()) {
            if (metadata.value(key).isNull())
                metadata.insert(key, common.value(key));
        }
    }

    // add duration
    const CMTime time = [asset duration];
    const qint64 duration =  static_cast<qint64>(float(time.value) / float(time.timescale) * 1000.0f);
    metadata.insert(QMediaMetaData::Duration, duration);

    // add creation date from asset if not already extracted from metadata items
    // (e.g. MP4 mvhd creation_time is only available via asset.creationDate)
    if (metadata.value(QMediaMetaData::Date).isNull()) {
        AVMetadataItem *creationDate = asset.creationDate;
        if (creationDate) {
            NSDate *dateValue = creationDate.dateValue;
            if (dateValue) {
                QDateTime dt = QDateTime::fromNSDate(dateValue);
                if (dt.isValid())
                    metadata.insert(QMediaMetaData::Date, dt);
            }
        }
    }

    std::optional<QtVideo::Rotation> rotationData;
    std::optional<QSize> resolutionData;
    QSemaphore sem(0);
    [asset loadTracksWithMediaType:AVMediaTypeVideo
                 completionHandler:[&](NSArray<AVAssetTrack *> *tracks, NSError *error) {
        if (!error && tracks && tracks.count > 0) {
            // only check the first video track
            AVAssetTrack *videoTrack = tracks[0];

            // add orientation
            QtVideo::Rotation rotation;
            bool mirrored = false;
            AVFMediaPlayer::videoOrientationForAssetTrack(videoTrack, rotation, mirrored);
            rotationData = rotation;

            // add resolution (coded frame size, without PAR adjustment)
            NSArray *formatDescriptions = [videoTrack formatDescriptions];
            if (formatDescriptions.count > 0) {
                const auto *desc = (__bridge CMVideoFormatDescriptionRef)formatDescriptions[0];
                CMVideoDimensions dims = CMVideoFormatDescriptionGetDimensions(desc);
                if (dims.width > 0 && dims.height > 0)
                    resolutionData = QSize(dims.width, dims.height);
            }
        }
        sem.release();
    }];

    if (!sem.try_acquire_for(5s)) {
        qWarning() << "Timed out waiting for video tracks to load, proceeding without orientation "
                      "metadata.";
        return metadata;
    }

    if (rotationData)
        metadata.insert(QMediaMetaData::Orientation, int(*rotationData));

    if (resolutionData)
        metadata.insert(QMediaMetaData::Resolution, *resolutionData);

    return metadata;
}

QMediaMetaData AVFMetaData::fromAssetTrack(AVAssetTrack *asset)
{
    QMediaMetaData metadata = fromAVMetadata([asset metadata]);
    if ([asset.mediaType isEqualToString:AVMediaTypeAudio]) {
        if (metadata.value(QMediaMetaData::Language).isNull()) {
            auto *languageCode = asset.languageCode;
            if (languageCode) {
                // languageCode is encoded as ISO 639-2, which QLocale does not handle.
                // Convert it to 639-1 first.
                QCFString lang = CFLocaleCreateCanonicalLanguageIdentifierFromString(
                        kCFAllocatorDefault, (__bridge CFStringRef)languageCode);
                metadata.insert(QMediaMetaData::Language, QLocale::codeToLanguage(QString{ lang }));
            }
        }
    }
    if ([asset.mediaType isEqualToString:AVMediaTypeVideo]) {
        // add orientation
        if (metadata.value(QMediaMetaData::Orientation).isNull()) {
            QtVideo::Rotation angle = QtVideo::Rotation::None;
            bool mirrored;
            AVFMediaPlayer::videoOrientationForAssetTrack(asset, angle, mirrored);
            Q_UNUSED(mirrored);
            metadata.insert(QMediaMetaData::Orientation, int(angle));
        }

        // add HDR content
        if (metadata.value(QMediaMetaData::HasHdrContent).isNull()) {
            auto hasHdrContent = false;

            NSArray *formatDescriptions = [asset formatDescriptions];
            for (id formatDescription in formatDescriptions) {
                NSDictionary *extensions = (__bridge NSDictionary *)CMFormatDescriptionGetExtensions((CMFormatDescriptionRef)formatDescription);
                NSString *transferFunction = extensions[(__bridge NSString *)kCMFormatDescriptionExtension_TransferFunction];
                if ([transferFunction isEqualToString:(__bridge NSString *)kCVImageBufferTransferFunction_SMPTE_ST_2084_PQ]
                    || [transferFunction isEqualToString:(__bridge NSString *)kCVImageBufferTransferFunction_ITU_R_2100_HLG]) {
                    hasHdrContent = true;
                    break;
                }
            }

            metadata.insert(QMediaMetaData::HasHdrContent, hasHdrContent);
        }
    }
    return metadata;
}

static AVMutableMetadataItem *setAVMetadataItemForKey(QMediaMetaData::Key key, const QVariant &value,
                                                      AVMetadataKeySpace keySpace = AVMetadataKeySpaceCommon)
{
    AVMetadataIdentifier identifier = toIdentifier(key, keySpace);
    if (!identifier.length)
        return nil;

    AVMutableMetadataItem *item = [AVMutableMetadataItem metadataItem];
    item.keySpace = keySpace;
    item.identifier = identifier;

    switch (key) {
    case QMediaMetaData::ThumbnailImage:
    case QMediaMetaData::CoverArtImage: {
#if defined(Q_OS_MACOS)
        QImage img = value.value<QImage>();
        if (!img.isNull()) {
            QByteArray arr;
            QBuffer buffer(&arr);
            buffer.open(QIODevice::WriteOnly);
            img.save(&buffer);
            NSData *data = arr.toNSData();
            NSImage *nsImg = [[NSImage alloc] initWithData:data];
            item.value = nsImg;
            [nsImg release];
        }
#endif
        break;
    }
    case QMediaMetaData::FileFormat: {
        QMediaFormat::FileFormat qtFormat = value.value<QMediaFormat::FileFormat>();
        AVFileType avFormat = QDarwinFormatInfo::avFileTypeForContainerFormat(qtFormat);
        item.value = avFormat;
        break;
    }
    case QMediaMetaData::Language: {
        QString lang = QLocale::languageToCode(value.value<QLocale::Language>());
        if (!lang.isEmpty())
            item.value = lang.toNSString();
        break;
    }
    case QMediaMetaData::Orientation: {
        bool ok;
        int rotation = value.toInt(&ok);
        if (ok)
            item.value = [NSNumber numberWithInt:rotation];
    }
    default: {
        switch (value.typeId()) {
        case QMetaType::QString: {
            item.value = value.toString().toNSString();
            break;
        }
        case QMetaType::Int: {
            item.value = [NSNumber numberWithInt:value.toInt()];
            break;
        }
        case QMetaType::LongLong: {
            item.value = [NSNumber numberWithLongLong:value.toLongLong()];
            break;
        }
        case QMetaType::Double: {
            item.value = [NSNumber numberWithDouble:value.toDouble()];
            break;
        }
        case QMetaType::QDate:
        case QMetaType::QDateTime: {
            item.value = value.toDateTime().toNSDate();
            break;
        }
        case QMetaType::QUrl: {
            item.value = value.toUrl().toNSURL();
            break;
        }
        default:
            break;
        }
    }
    }

    return item;
}

NSMutableArray<AVMetadataItem *> *AVFMetaData::toAVMetadataForFormat(QMediaMetaData metadata, AVFileType format)
{
    NSMutableArray<AVMetadataKeySpace> *keySpaces = [NSMutableArray<AVMetadataKeySpace> array];
    if (format == AVFileTypeAppleM4A) {
        [keySpaces addObject:AVMetadataKeySpaceiTunes];
    } else if (format == AVFileTypeMPEGLayer3) {
        [keySpaces addObject:AVMetadataKeySpaceID3];
        [keySpaces addObject:AVMetadataKeySpaceiTunes];
    } else if (format == AVFileTypeQuickTimeMovie) {
        [keySpaces addObject:AVMetadataKeySpaceQuickTimeMetadata];
    } else {
        [keySpaces addObject:AVMetadataKeySpaceCommon];
    }
    NSMutableArray<AVMetadataItem *> *avMetaDataArr = [NSMutableArray array];
    for (const auto &key : metadata.keys()) {
        for (NSUInteger i = 0; i < [keySpaces count]; i++) {
            const QVariant &value = metadata.value(key);
            // set format-specific metadata
            AVMetadataItem *item = setAVMetadataItemForKey(key, value, keySpaces[i]);
            if (item)
                [avMetaDataArr addObject:item];
        }
    }
    return avMetaDataArr;
}
