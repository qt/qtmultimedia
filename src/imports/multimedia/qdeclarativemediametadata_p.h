/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef QDECLARATIVEMEDIAMETADATA_P_H
#define QDECLARATIVEMEDIAMETADATA_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qmetadatareadercontrol.h>

#include <qdeclarative.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QDeclarativeMediaMetaData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant title READ title NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant subTitle READ subTitle NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant author READ author NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant comment READ comment NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant description READ description NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant category READ category NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant genre READ genre NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant year READ year NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant date READ date NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant userRating READ userRating NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant keywords READ keywords NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant language READ language NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant publisher READ publisher NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant copyright READ copyright NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant parentalRating READ parentalRating NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant ratingOrganization READ ratingOrganization NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant size READ size NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant mediaType READ mediaType NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant duration READ duration NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant audioBitRate READ audioBitRate NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant audioCodec READ audioCodec NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant averageLevel READ averageLevel NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant channelCount READ channelCount NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant peakValue READ peakValue NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant sampleRate READ sampleRate NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant albumTitle READ albumTitle NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant albumArtist READ albumArtist NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant contributingArtist READ contributingArtist NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant composer READ composer NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant conductor READ conductor NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant lyrics READ lyrics NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant mood READ mood NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant trackNumber READ trackNumber NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant trackCount READ trackCount NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant coverArtUrlSmall READ coverArtUrlSmall NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant coverArtUrlLarge READ coverArtUrlLarge NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant resolution READ resolution NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant pixelAspectRatio READ pixelAspectRatio NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant videoFrameRate READ videoFrameRate NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant videoBitRate READ videoBitRate NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant videoCodec READ videoCodec NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant posterUrl READ posterUrl NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant chapterNumber READ chapterNumber NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant director READ director NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant leadPerformer READ leadPerformer NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant writer READ writer NOTIFY metaDataChanged)
public:
    QDeclarativeMediaMetaData(QMetaDataReaderControl *control, QObject *parent = 0)
        : QObject(parent)
        , m_control(control)
    {
    }

    QVariant title() const { return m_control->metaData(QtMultimedia::MetaData::Title); }
    QVariant subTitle() const { return m_control->metaData(QtMultimedia::MetaData::SubTitle); }
    QVariant author() const { return m_control->metaData(QtMultimedia::MetaData::Author); }
    QVariant comment() const { return m_control->metaData(QtMultimedia::MetaData::Comment); }
    QVariant description() const { return m_control->metaData(QtMultimedia::MetaData::Description); }
    QVariant category() const { return m_control->metaData(QtMultimedia::MetaData::Category); }
    QVariant genre() const { return m_control->metaData(QtMultimedia::MetaData::Genre); }
    QVariant year() const { return m_control->metaData(QtMultimedia::MetaData::Year); }
    QVariant date() const { return m_control->metaData(QtMultimedia::MetaData::Date); }
    QVariant userRating() const { return m_control->metaData(QtMultimedia::MetaData::UserRating); }
    QVariant keywords() const { return m_control->metaData(QtMultimedia::MetaData::Keywords); }
    QVariant language() const { return m_control->metaData(QtMultimedia::MetaData::Language); }
    QVariant publisher() const { return m_control->metaData(QtMultimedia::MetaData::Publisher); }
    QVariant copyright() const { return m_control->metaData(QtMultimedia::MetaData::Copyright); }
    QVariant parentalRating() const { return m_control->metaData(QtMultimedia::MetaData::ParentalRating); }
    QVariant ratingOrganization() const {
        return m_control->metaData(QtMultimedia::MetaData::RatingOrganization); }
    QVariant size() const { return m_control->metaData(QtMultimedia::MetaData::Size); }
    QVariant mediaType() const { return m_control->metaData(QtMultimedia::MetaData::MediaType); }
    QVariant duration() const { return m_control->metaData(QtMultimedia::MetaData::Duration); }
    QVariant audioBitRate() const { return m_control->metaData(QtMultimedia::MetaData::AudioBitRate); }
    QVariant audioCodec() const { return m_control->metaData(QtMultimedia::MetaData::AudioCodec); }
    QVariant averageLevel() const { return m_control->metaData(QtMultimedia::MetaData::AverageLevel); }
    QVariant channelCount() const { return m_control->metaData(QtMultimedia::MetaData::ChannelCount); }
    QVariant peakValue() const { return m_control->metaData(QtMultimedia::MetaData::PeakValue); }
    QVariant sampleRate() const { return m_control->metaData(QtMultimedia::MetaData::SampleRate); }
    QVariant albumTitle() const { return m_control->metaData(QtMultimedia::MetaData::AlbumTitle); }
    QVariant albumArtist() const { return m_control->metaData(QtMultimedia::MetaData::AlbumArtist); }
    QVariant contributingArtist() const {
        return m_control->metaData(QtMultimedia::MetaData::ContributingArtist); }
    QVariant composer() const { return m_control->metaData(QtMultimedia::MetaData::Composer); }
    QVariant conductor() const { return m_control->metaData(QtMultimedia::MetaData::Conductor); }
    QVariant lyrics() const { return m_control->metaData(QtMultimedia::MetaData::Lyrics); }
    QVariant mood() const { return m_control->metaData(QtMultimedia::MetaData::Mood); }
    QVariant trackNumber() const { return m_control->metaData(QtMultimedia::MetaData::TrackNumber); }
    QVariant trackCount() const { return m_control->metaData(QtMultimedia::MetaData::TrackCount); }
    QVariant coverArtUrlSmall() const {
        return m_control->metaData(QtMultimedia::MetaData::CoverArtUrlSmall); }
    QVariant coverArtUrlLarge() const {
        return m_control->metaData(QtMultimedia::MetaData::CoverArtUrlLarge); }
    QVariant resolution() const { return m_control->metaData(QtMultimedia::MetaData::Resolution); }
    QVariant pixelAspectRatio() const {
        return m_control->metaData(QtMultimedia::MetaData::PixelAspectRatio); }
    QVariant videoFrameRate() const { return m_control->metaData(QtMultimedia::MetaData::VideoFrameRate); }
    QVariant videoBitRate() const { return m_control->metaData(QtMultimedia::MetaData::VideoBitRate); }
    QVariant videoCodec() const { return m_control->metaData(QtMultimedia::MetaData::VideoCodec); }
    QVariant posterUrl() const { return m_control->metaData(QtMultimedia::MetaData::PosterUrl); }
    QVariant chapterNumber() const { return m_control->metaData(QtMultimedia::MetaData::ChapterNumber); }
    QVariant director() const { return m_control->metaData(QtMultimedia::MetaData::Director); }
    QVariant leadPerformer() const { return m_control->metaData(QtMultimedia::MetaData::LeadPerformer); }
    QVariant writer() const { return m_control->metaData(QtMultimedia::MetaData::Writer); }

Q_SIGNALS:
    void metaDataChanged();

private:
    QMetaDataReaderControl *m_control;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QT_PREPEND_NAMESPACE(QDeclarativeMediaMetaData))

QT_END_HEADER

#endif
