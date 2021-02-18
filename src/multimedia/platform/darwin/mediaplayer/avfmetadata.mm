/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "avfmetadata_p.h"

#include <QtMultimedia/qmediametadata.h>

#import <AVFoundation/AVFoundation.h>

QT_USE_NAMESPACE

static std::optional<QMediaMetaData::Key> itemKey(AVMetadataItem *item)
{
    NSString *keyString = [item commonKey];
//    qDebug() << "metadatakey:" << QString::fromNSString(keyString)
//             << QString::fromNSString([item identifier]) << QString::fromNSString([item description]);

    if (keyString.length != 0) {
        if ([keyString isEqualToString:AVMetadataCommonKeyTitle]) {
            return QMediaMetaData::Title;
//        } else if ([keyString isEqualToString: AVMetadataCommonKeySubject]) {
//            return QMediaMetaData::SubTitle;
        } else if ([keyString isEqualToString: AVMetadataCommonKeyDescription]) {
            return QMediaMetaData::Description;
        } else if ([keyString isEqualToString: AVMetadataCommonKeyPublisher]) {
            return QMediaMetaData::Publisher;
        } else if ([keyString isEqualToString: AVMetadataCommonKeyCreationDate]) {
            return QMediaMetaData::Date;
        } else if ([keyString isEqualToString: AVMetadataCommonKeyType]) {
            return QMediaMetaData::MediaType;
        } else if ([keyString isEqualToString: AVMetadataCommonKeyLanguage]) {
            return QMediaMetaData::Language;
        } else if ([keyString isEqualToString: AVMetadataCommonKeyCopyrights]) {
            return QMediaMetaData::Copyright;
        } else if ([keyString isEqualToString: AVMetadataCommonKeyAlbumName]) {
            return QMediaMetaData::AlbumTitle;
        } else if ([keyString isEqualToString: AVMetadataCommonKeyAuthor]) {
            return QMediaMetaData::Author;
        } else if ([keyString isEqualToString: AVMetadataCommonKeyArtist]) {
            return QMediaMetaData::ContributingArtist;
//        } else if ([keyString isEqualToString: AVMetadataCommonKeyArtwork]) {
//            return QMediaMetaData::PosterUrl;
        }
    }

    // check by identifier
    NSString *id = [item identifier];
    if (!id)
        return std::nullopt;

    if ([id isEqualToString: AVMetadataIdentifieriTunesMetadataUserComment] ||
        [id isEqualToString: AVMetadataIdentifierID3MetadataComments] ||
            [id isEqualToString: AVMetadataIdentifierQuickTimeMetadataComment])
        return QMediaMetaData::Comment;

    if ([id isEqualToString: AVMetadataIdentifierID3MetadataDate] ||
        [id isEqualToString: AVMetadataIdentifierISOUserDataDate])
        return QMediaMetaData::Date;


    return std::nullopt;
}

QMediaMetaData AVFMetaData::fromAsset(AVAsset *asset)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    QMediaMetaData metaData;

    // TODO: also process ID3, iTunes and QuickTime metadata

    NSArray *metadataItems = [asset metadata];
    for (AVMetadataItem* item in metadataItems) {
        auto key = itemKey(item);
        if (!key)
            continue;

        const QString value = QString::fromNSString([item stringValue]);
        if (!value.isNull())
            metaData.insert(*key, value);
    }

    // add duration
    const CMTime time = [asset duration];
    const qint64 duration =  static_cast<qint64>(float(time.value) / float(time.timescale) * 1000.0f);
    metaData.insert(QMediaMetaData::Duration, duration);

    return metaData;
}
