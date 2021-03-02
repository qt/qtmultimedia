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

#include <QtQml/qqml.h>
#include <QtMultimedia/qmediametadata.h>
#include "qmediametadata.h"

QT_BEGIN_NAMESPACE

class QDeclarativeMediaMetaData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant title READ title WRITE setTitle NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant subTitle READ subTitle WRITE setSubTitle NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant author READ author WRITE setAuthor NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant comment READ comment WRITE setComment NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant description READ description WRITE setDescription NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant category READ category WRITE setCategory NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant genre READ genre WRITE setGenre NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant year READ year WRITE setYear NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant date READ date WRITE setDate NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant userRating READ userRating WRITE setUserRating NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant keywords READ keywords WRITE setKeywords NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant language READ language WRITE setLanguage NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant publisher READ publisher WRITE setPublisher NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant copyright READ copyright WRITE setCopyright NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant parentalRating READ parentalRating WRITE setParentalRating NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant ratingOrganization READ ratingOrganization WRITE setRatingOrganization NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant size READ size WRITE setSize NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant mediaType READ mediaType WRITE setMediaType NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant duration READ duration WRITE setDuration NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant audioBitRate READ audioBitRate WRITE setAudioBitRate NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant audioCodec READ audioCodec WRITE setAudioCodec NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant averageLevel READ averageLevel WRITE setAverageLevel NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant channelCount READ channelCount WRITE setChannelCount NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant peakValue READ peakValue WRITE setPeakValue NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant sampleRate READ sampleRate WRITE setSampleRate NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant albumTitle READ albumTitle WRITE setAlbumTitle NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant albumArtist READ albumArtist WRITE setAlbumArtist NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant contributingArtist READ contributingArtist WRITE setContributingArtist NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant composer READ composer WRITE setComposer NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant conductor READ conductor WRITE setConductor NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant lyrics READ lyrics WRITE setLyrics NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant mood READ mood WRITE setMood NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant trackNumber READ trackNumber WRITE setTrackNumber NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant trackCount READ trackCount WRITE setTrackCount NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant coverArtUrlSmall READ coverArtUrlSmall WRITE setCoverArtUrlSmall NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant coverArtUrlLarge READ coverArtUrlLarge WRITE setCoverArtUrlLarge NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant resolution READ resolution WRITE setResolution NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant videoFrameRate READ videoFrameRate WRITE setVideoFrameRate NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant videoBitRate READ videoBitRate WRITE setVideoBitRate NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant videoCodec READ videoCodec WRITE setVideoCodec NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant posterUrl READ posterUrl WRITE setPosterUrl NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant chapterNumber READ chapterNumber WRITE setChapterNumber NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant director READ director WRITE setDirector NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant leadPerformer READ leadPerformer WRITE setLeadPerformer NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant writer READ writer WRITE setWriter NOTIFY metaDataChanged)

//    Q_PROPERTY(QVariant cameraManufacturer READ cameraManufacturer WRITE setCameraManufacturer NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant cameraModel READ cameraModel WRITE setCameraModel NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant event READ event WRITE setEvent NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant subject READ subject WRITE setSubject NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant orientation READ orientation WRITE setOrientation NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant exposureTime READ exposureTime WRITE setExposureTime NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant fNumber READ fNumber WRITE setFNumber NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant exposureProgram READ exposureProgram WRITE setExposureProgram NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant isoSpeedRatings READ isoSpeedRatings WRITE setISOSpeedRatings NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant exposureBiasValue READ exposureBiasValue WRITE setExposureBiasValue NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant dateTimeOriginal READ dateTimeOriginal WRITE setDateTimeOriginal NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant dateTimeDigitized READ dateTimeDigitized WRITE setDateTimeDigitized NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant subjectDistance READ subjectDistance WRITE setSubjectDistance NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant lightSource READ lightSource WRITE setLightSource NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant flash READ flash WRITE setFlash NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant focalLength READ focalLength WRITE setFocalLength NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant exposureMode READ exposureMode WRITE setExposureMode NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant whiteBalance READ whiteBalance WRITE setWhiteBalance NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant digitalZoomRatio READ digitalZoomRatio WRITE setDigitalZoomRatio NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant focalLengthIn35mmFilm READ focalLengthIn35mmFilm WRITE setFocalLengthIn35mmFilm NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant sceneCaptureType READ sceneCaptureType WRITE setSceneCaptureType NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant gainControl READ gainControl WRITE setGainControl NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant contrast READ contrast WRITE setContrast NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant saturation READ saturation WRITE setSaturation NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant sharpness READ sharpness WRITE setSharpness NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant deviceSettingDescription READ deviceSettingDescription WRITE setDeviceSettingDescription NOTIFY metaDataChanged)

//    Q_PROPERTY(QVariant gpsLatitude READ gpsLatitude WRITE setGPSLatitude NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant gpsLongitude READ gpsLongitude WRITE setGPSLongitude NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant gpsAltitude READ gpsAltitude WRITE setGPSAltitude NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant gpsTimeStamp READ gpsTimeStamp WRITE setGPSTimeStamp NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant gpsSatellites READ gpsSatellites WRITE setGPSSatellites NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant gpsStatus READ gpsStatus WRITE setGPSStatus NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant gpsDOP READ gpsDOP WRITE setGPSDOP NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant gpsSpeed READ gpsSpeed WRITE setGPSSpeed NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant gpsTrack READ gpsTrack WRITE setGPSTrack NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant gpsTrackRef READ gpsTrackRef WRITE setGPSTrackRef NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant gpsImgDirection READ gpsImgDirection WRITE setGPSImgDirection NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant gpsImgDirectionRef READ gpsImgDirectionRef WRITE setGPSImgDirectionRef NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant gpsMapDatum READ gpsMapDatum WRITE setGPSMapDatum NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant gpsProcessingMethod READ gpsProcessingMethod WRITE setGPSProcessingMethod NOTIFY metaDataChanged)
//    Q_PROPERTY(QVariant gpsAreaInformation READ gpsAreaInformation WRITE setGPSAreaInformation NOTIFY metaDataChanged)

public:
    QDeclarativeMediaMetaData(QObject *player, QObject *parent = 0)
        : QObject(parent)
        , m_mediaSource(player)
    {
        connect(player, SIGNAL(metaDataChanged()), this, SIGNAL(metaDataChanged()));
        connect(player, SIGNAL(metaDataChanged()), this, SLOT(updateMetaData()));
        auto *mo = player->metaObject();
        int idx = mo->indexOfProperty("metaData");
        if (idx >= 0)
            m_property = mo->property(idx);
        updateMetaData();
    }

    ~QDeclarativeMediaMetaData()
    {
    }

    QVariant title() const { return metaData.value(QMediaMetaData::Title); }
    void setTitle(const QVariant &title) { setMetaData(QMediaMetaData::Title, title); }
    QVariant author() const { return metaData.value(QMediaMetaData::Author); }
    void setAuthor(const QVariant &author) { setMetaData(QMediaMetaData::Author, author); }
    QVariant comment() const { return metaData.value(QMediaMetaData::Comment); }
    void setComment(const QVariant &comment) { setMetaData(QMediaMetaData::Comment, comment); }
    QVariant description() const { return metaData.value(QMediaMetaData::Description); }
    void setDescription(const QVariant &description) {
        setMetaData(QMediaMetaData::Description, description); }
    QVariant genre() const { return metaData.value(QMediaMetaData::Genre); }
    void setGenre(const QVariant &genre) { setMetaData(QMediaMetaData::Genre, genre); }
    QVariant year() const { return metaData.value(QMediaMetaData::Year); }
    void setYear(const QVariant &year) { setMetaData(QMediaMetaData::Year, year); }
    QVariant date() const { return metaData.value(QMediaMetaData::Date); }
    void setDate(const QVariant &date) { setMetaData(QMediaMetaData::Date, date); }
//    QVariant userRating() const { return metaData.value(QMediaMetaData::UserRating); }
//    void setUserRating(const QVariant &rating) { setMetaData(QMediaMetaData::UserRating, rating); }
//    QVariant keywords() const { return metaData.value(QMediaMetaData::Keywords); }
//    void setKeywords(const QVariant &keywords) { setMetaData(QMediaMetaData::Keywords, keywords); }
//    QVariant language() const { return metaData.value(QMediaMetaData::Language); }
//    void setLanguage(const QVariant &language) { setMetaData(QMediaMetaData::Language, language); }
//    QVariant publisher() const { return metaData.value(QMediaMetaData::Publisher); }
//    void setPublisher(const QVariant &publisher) {
//        setMetaData(QMediaMetaData::Publisher, publisher); }
//    QVariant copyright() const { return metaData.value(QMediaMetaData::Copyright); }
//    void setCopyright(const QVariant &copyright) {
//        setMetaData(QMediaMetaData::Copyright, copyright); }
//    QVariant parentalRating() const { return metaData.value(QMediaMetaData::ParentalRating); }
//    void setParentalRating(const QVariant &rating) {
//        setMetaData(QMediaMetaData::ParentalRating, rating); }
//    QVariant ratingOrganization() const {
//        return metaData.value(QMediaMetaData::RatingOrganization); }
//    void setRatingOrganization(const QVariant &organization) {
//        setMetaData(QMediaMetaData::RatingOrganization, organization); }
//    QVariant size() const { return metaData.value(QMediaMetaData::Size); }
//    void setSize(const QVariant &size) { setMetaData(QMediaMetaData::Size, size); }
//    QVariant mediaType() const { return metaData.value(QMediaMetaData::MediaType); }
//    void setMediaType(const QVariant &type) { setMetaData(QMediaMetaData::MediaType, type); }
//    QVariant duration() const { return metaData.value(QMediaMetaData::Duration); }
//    void setDuration(const QVariant &duration) { setMetaData(QMediaMetaData::Duration, duration); }
//    QVariant audioBitRate() const { return metaData.value(QMediaMetaData::AudioBitRate); }
//    void setAudioBitRate(const QVariant &rate) { setMetaData(QMediaMetaData::AudioBitRate, rate); }
//    QVariant audioCodec() const { return metaData.value(QMediaMetaData::AudioCodec); }
//    void setAudioCodec(const QVariant &codec) { setMetaData(QMediaMetaData::AudioCodec, codec); }
//    QVariant averageLevel() const { return metaData.value(QMediaMetaData::AverageLevel); }
//    void setAverageLevel(const QVariant &level) {
//        setMetaData(QMediaMetaData::AverageLevel, level); }
//    QVariant channelCount() const { return metaData.value(QMediaMetaData::ChannelCount); }
//    void setChannelCount(const QVariant &count) {
//        setMetaData(QMediaMetaData::ChannelCount, count); }
//    QVariant peakValue() const { return metaData.value(QMediaMetaData::PeakValue); }
//    void setPeakValue(const QVariant &value) { setMetaData(QMediaMetaData::PeakValue, value); }
//    QVariant sampleRate() const { return metaData.value(QMediaMetaData::SampleRate); }
//    void setSampleRate(const QVariant &rate) { setMetaData(QMediaMetaData::SampleRate, rate); }
//    QVariant albumTitle() const { return metaData.value(QMediaMetaData::AlbumTitle); }
//    void setAlbumTitle(const QVariant &title) { setMetaData(QMediaMetaData::AlbumTitle, title); }
//    QVariant albumArtist() const { return metaData.value(QMediaMetaData::AlbumArtist); }
//    void setAlbumArtist(const QVariant &artist) {
//        setMetaData(QMediaMetaData::AlbumArtist, artist); }
//    QVariant contributingArtist() const {
//        return metaData.value(QMediaMetaData::ContributingArtist); }
//    void setContributingArtist(const QVariant &artist) {
//        setMetaData(QMediaMetaData::ContributingArtist, artist); }
//    QVariant composer() const { return metaData.value(QMediaMetaData::Composer); }
//    void setComposer(const QVariant &composer) { setMetaData(QMediaMetaData::Composer, composer); }
//    QVariant conductor() const { return metaData.value(QMediaMetaData::Conductor); }
//    void setConductor(const QVariant &conductor) {
//        setMetaData(QMediaMetaData::Conductor, conductor); }
//    QVariant lyrics() const { return metaData.value(QMediaMetaData::Lyrics); }
//    void setLyrics(const QVariant &lyrics) { setMetaData(QMediaMetaData::Lyrics, lyrics); }
//    QVariant mood() const { return metaData.value(QMediaMetaData::Mood); }
//    void setMood(const QVariant &mood) { setMetaData(QMediaMetaData::Mood, mood); }
//    QVariant trackNumber() const { return metaData.value(QMediaMetaData::TrackNumber); }
//    void setTrackNumber(const QVariant &track) { setMetaData(QMediaMetaData::TrackNumber, track); }
//    QVariant trackCount() const { return metaData.value(QMediaMetaData::TrackCount); }
//    void setTrackCount(const QVariant &count) { setMetaData(QMediaMetaData::TrackCount, count); }
//    QVariant coverArtUrlSmall() const {
//        return metaData.value(QMediaMetaData::CoverArtUrlSmall); }
//    void setCoverArtUrlSmall(const QVariant &url) {
//        setMetaData(QMediaMetaData::CoverArtUrlSmall, url); }
//    QVariant coverArtUrlLarge() const {
//        return metaData.value(QMediaMetaData::CoverArtUrlLarge); }
//    void setCoverArtUrlLarge(const QVariant &url) {
//        setMetaData(QMediaMetaData::CoverArtUrlLarge, url); }
//    QVariant resolution() const { return metaData.value(QMediaMetaData::Resolution); }
//    void setResolution(const QVariant &resolution) {
//        setMetaData(QMediaMetaData::Resolution, resolution); }
//    QVariant videoFrameRate() const { return metaData.value(QMediaMetaData::VideoFrameRate); }
//    void setVideoFrameRate(const QVariant &rate) {
//        setMetaData(QMediaMetaData::VideoFrameRate, rate); }
//    QVariant videoBitRate() const { return metaData.value(QMediaMetaData::VideoBitRate); }
//    void setVideoBitRate(const QVariant &rate) {
//        setMetaData(QMediaMetaData::VideoBitRate, rate); }
//    QVariant videoCodec() const { return metaData.value(QMediaMetaData::VideoCodec); }
//    void setVideoCodec(const QVariant &codec) {
//        setMetaData(QMediaMetaData::VideoCodec, codec); }
//    QVariant posterUrl() const { return metaData.value(QMediaMetaData::PosterUrl); }
//    void setPosterUrl(const QVariant &url) {
//        setMetaData(QMediaMetaData::PosterUrl, url); }
//    QVariant chapterNumber() const { return metaData.value(QMediaMetaData::ChapterNumber); }
//    void setChapterNumber(const QVariant &chapter) {
//        setMetaData(QMediaMetaData::ChapterNumber, chapter); }
//    QVariant director() const { return metaData.value(QMediaMetaData::Director); }
//    void setDirector(const QVariant &director) { setMetaData(QMediaMetaData::Director, director); }
//    QVariant leadPerformer() const { return metaData.value(QMediaMetaData::LeadPerformer); }
//    void setLeadPerformer(const QVariant &performer) {
//        setMetaData(QMediaMetaData::LeadPerformer, performer); }
//    QVariant writer() const { return metaData.value(QMediaMetaData::Writer); }
//    void setWriter(const QVariant &writer) { setMetaData(QMediaMetaData::Writer, writer); }

//    QVariant cameraManufacturer() const {
//        return metaData.value(QMediaMetaData::CameraManufacturer); }
//    void setCameraManufacturer(const QVariant &manufacturer) {
//        setMetaData(QMediaMetaData::CameraManufacturer, manufacturer); }
//    QVariant cameraModel() const { return metaData.value(QMediaMetaData::CameraModel); }
//    void setCameraModel(const QVariant &model) { setMetaData(QMediaMetaData::CameraModel, model); }

//QT_WARNING_PUSH
//QT_WARNING_DISABLE_GCC("-Woverloaded-virtual")
//QT_WARNING_DISABLE_CLANG("-Woverloaded-virtual")
//    QVariant event() const { return metaData.value(QMediaMetaData::Event); }
//QT_WARNING_POP

//    void setEvent(const QVariant &event) { setMetaData(QMediaMetaData::Event, event); }
//    QVariant subject() const { return metaData.value(QMediaMetaData::Subject); }
//    void setSubject(const QVariant &subject) { setMetaData(QMediaMetaData::Subject, subject); }
//    QVariant orientation() const { return metaData.value(QMediaMetaData::Orientation); }
//    void setOrientation(const QVariant &orientation) {
//        setMetaData(QMediaMetaData::Orientation, orientation); }
//    QVariant exposureTime() const { return metaData.value(QMediaMetaData::ExposureTime); }
//    void setExposureTime(const QVariant &time) { setMetaData(QMediaMetaData::ExposureTime, time); }
//    QVariant fNumber() const { return metaData.value(QMediaMetaData::FNumber); }
//    void setFNumber(const QVariant &number) { setMetaData(QMediaMetaData::FNumber, number); }
//    QVariant exposureProgram() const {
//        return metaData.value(QMediaMetaData::ExposureProgram); }
//    void setExposureProgram(const QVariant &program) {
//        setMetaData(QMediaMetaData::ExposureProgram, program); }
//    QVariant isoSpeedRatings() const {
//        return metaData.value(QMediaMetaData::ISOSpeedRatings); }
//    void setISOSpeedRatings(const QVariant &ratings) {
//        setMetaData(QMediaMetaData::ISOSpeedRatings, ratings); }
//    QVariant exposureBiasValue() const {
//        return metaData.value(QMediaMetaData::ExposureBiasValue); }
//    void setExposureBiasValue(const QVariant &bias) {
//        setMetaData(QMediaMetaData::ExposureBiasValue, bias); }
//    QVariant dateTimeOriginal() const {
//        return metaData.value(QMediaMetaData::DateTimeOriginal); }
//    void setDateTimeOriginal(const QVariant &dateTime) {
//        setMetaData(QMediaMetaData::DateTimeOriginal, dateTime); }
//    QVariant dateTimeDigitized() const {
//        return metaData.value(QMediaMetaData::DateTimeDigitized); }
//    void setDateTimeDigitized(const QVariant &dateTime) {
//        setMetaData(QMediaMetaData::DateTimeDigitized, dateTime); }
//    QVariant subjectDistance() const {
//        return metaData.value(QMediaMetaData::SubjectDistance); }
//    void setSubjectDistance(const QVariant &distance) {
//        setMetaData(QMediaMetaData::SubjectDistance, distance); }
//    QVariant lightSource() const { return metaData.value(QMediaMetaData::LightSource); }
//    void setLightSource(const QVariant &source) {
//        setMetaData(QMediaMetaData::LightSource, source); }
//    QVariant flash() const { return metaData.value(QMediaMetaData::Flash); }
//    void setFlash(const QVariant &flash) { setMetaData(QMediaMetaData::Flash, flash); }
//    QVariant focalLength() const { return metaData.value(QMediaMetaData::FocalLength); }
//    void setFocalLength(const QVariant &length) {
//        setMetaData(QMediaMetaData::FocalLength, length); }
//    QVariant exposureMode() const { return metaData.value(QMediaMetaData::ExposureMode); }
//    void setExposureMode(const QVariant &mode) {
//        setMetaData(QMediaMetaData::ExposureMode, mode); }
//    QVariant whiteBalance() const { return metaData.value(QMediaMetaData::WhiteBalance); }
//    void setWhiteBalance(const QVariant &balance) {
//        setMetaData(QMediaMetaData::WhiteBalance, balance); }
//    QVariant digitalZoomRatio() const {
//        return metaData.value(QMediaMetaData::DigitalZoomRatio); }
//    void setDigitalZoomRatio(const QVariant &ratio) {
//        setMetaData(QMediaMetaData::DigitalZoomRatio, ratio); }
//    QVariant focalLengthIn35mmFilm() const {
//        return metaData.value(QMediaMetaData::FocalLengthIn35mmFilm); }
//    void setFocalLengthIn35mmFilm(const QVariant &length) {
//        setMetaData(QMediaMetaData::FocalLengthIn35mmFilm, length); }
//    QVariant sceneCaptureType() const {
//        return metaData.value(QMediaMetaData::SceneCaptureType); }
//    void setSceneCaptureType(const QVariant &type) {
//        setMetaData(QMediaMetaData::SceneCaptureType, type); }
//    QVariant gainControl() const { return metaData.value(QMediaMetaData::GainControl); }
//    void setGainControl(const QVariant &gain) { setMetaData(QMediaMetaData::GainControl, gain); }
//    QVariant contrast() const { return metaData.value(QMediaMetaData::Contrast); }
//    void setContrast(const QVariant &contrast) { setMetaData(QMediaMetaData::Contrast, contrast); }
//    QVariant saturation() const { return metaData.value(QMediaMetaData::Saturation); }
//    void setSaturation(const QVariant &saturation) {
//        setMetaData(QMediaMetaData::Saturation, saturation); }
//    QVariant sharpness() const { return metaData.value(QMediaMetaData::Sharpness); }
//    void setSharpness(const QVariant &sharpness) {
//        setMetaData(QMediaMetaData::Sharpness, sharpness); }
//    QVariant deviceSettingDescription() const {
//        return metaData.value(QMediaMetaData::DeviceSettingDescription); }
//    void setDeviceSettingDescription(const QVariant &description) {
//        setMetaData(QMediaMetaData::DeviceSettingDescription, description); }

//    QVariant gpsLatitude() const { return metaData.value(QMediaMetaData::GPSLatitude); }
//    void setGPSLatitude(const QVariant &latitude) {
//        setMetaData(QMediaMetaData::GPSLatitude, latitude); }
//    QVariant gpsLongitude() const { return metaData.value(QMediaMetaData::GPSLongitude); }
//    void setGPSLongitude(const QVariant &longitude) {
//        setMetaData(QMediaMetaData::GPSLongitude, longitude); }
//    QVariant gpsAltitude() const { return metaData.value(QMediaMetaData::GPSAltitude); }
//    void setGPSAltitude(const QVariant &altitude) {
//        setMetaData(QMediaMetaData::GPSAltitude, altitude); }
//    QVariant gpsTimeStamp() const { return metaData.value(QMediaMetaData::GPSTimeStamp); }
//    void setGPSTimeStamp(const QVariant &timestamp) {
//        setMetaData(QMediaMetaData::GPSTimeStamp, timestamp); }
//    QVariant gpsSatellites() const {
//        return metaData.value(QMediaMetaData::GPSSatellites); }
//    void setGPSSatellites(const QVariant &satellites) {
//        setMetaData(QMediaMetaData::GPSSatellites, satellites); }
//    QVariant gpsStatus() const { return metaData.value(QMediaMetaData::GPSStatus); }
//    void setGPSStatus(const QVariant &status) { setMetaData(QMediaMetaData::GPSStatus, status); }
//    QVariant gpsDOP() const { return metaData.value(QMediaMetaData::GPSDOP); }
//    void setGPSDOP(const QVariant &dop) { setMetaData(QMediaMetaData::GPSDOP, dop); }
//    QVariant gpsSpeed() const { return metaData.value(QMediaMetaData::GPSSpeed); }
//    void setGPSSpeed(const QVariant &speed) { setMetaData(QMediaMetaData::GPSSpeed, speed); }
//    QVariant gpsTrack() const { return metaData.value(QMediaMetaData::GPSTrack); }
//    void setGPSTrack(const QVariant &track) { setMetaData(QMediaMetaData::GPSTrack, track); }
//    QVariant gpsTrackRef() const { return metaData.value(QMediaMetaData::GPSTrackRef); }
//    void setGPSTrackRef(const QVariant &ref) { setMetaData(QMediaMetaData::GPSTrackRef, ref); }
//    QVariant gpsImgDirection() const {
//        return metaData.value(QMediaMetaData::GPSImgDirection); }
//    void setGPSImgDirection(const QVariant &direction) {
//        setMetaData(QMediaMetaData::GPSImgDirection, direction); }
//    QVariant gpsImgDirectionRef() const {
//        return metaData.value(QMediaMetaData::GPSImgDirectionRef); }
//    void setGPSImgDirectionRef(const QVariant &ref) {
//        setMetaData(QMediaMetaData::GPSImgDirectionRef, ref); }
//    QVariant gpsMapDatum() const { return metaData.value(QMediaMetaData::GPSMapDatum); }
//    void setGPSMapDatum(const QVariant &datum) {
//        setMetaData(QMediaMetaData::GPSMapDatum, datum); }
//    QVariant gpsProcessingMethod() const {
//        return metaData.value(QMediaMetaData::GPSProcessingMethod); }
//    void setGPSProcessingMethod(const QVariant &method) {
//        setMetaData(QMediaMetaData::GPSProcessingMethod, method); }
//    QVariant gpsAreaInformation() const {
//        return metaData.value(QMediaMetaData::GPSAreaInformation); }
//    void setGPSAreaInformation(const QVariant &information) {
//        setMetaData(QMediaMetaData::GPSAreaInformation, information); }

Q_SIGNALS:
    void metaDataChanged();

private:
    void setMetaData(QMediaMetaData::Key key, const QVariant &value)
    {
        metaData.insert(key, value);
        if (!m_property.isValid() || !m_property.isWritable())
            return;
        m_property.write(m_mediaSource, QVariant::fromValue(metaData));
    }

    void updateMetaData()
    {
        if (!m_property.isValid() || !m_property.isReadable())
            return;
        metaData = m_property.read(m_mediaSource).value<QMediaMetaData>();
    }

    QMetaProperty m_property;
    QObject *m_mediaSource;
    QMediaMetaData metaData;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QT_PREPEND_NAMESPACE(QDeclarativeMediaMetaData))

#endif
