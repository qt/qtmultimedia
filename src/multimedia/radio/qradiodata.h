/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QRADIODATA_H
#define QRADIODATA_H

#include <QtCore/qobject.h>

#include <QtMultimedia/qmediaobject.h>
#include <QtMultimedia/qmediabindableinterface.h>
#include <QtMultimedia/qmediaenumdebug.h>

QT_BEGIN_NAMESPACE


class QRadioDataPrivate;
class Q_MULTIMEDIA_EXPORT QRadioData : public QObject, public QMediaBindableInterface
{
    Q_OBJECT
    Q_PROPERTY(QString stationId READ stationId NOTIFY stationIdChanged)
    Q_PROPERTY(ProgramType programType READ programType NOTIFY programTypeChanged)
    Q_PROPERTY(QString programTypeName READ programTypeName NOTIFY programTypeNameChanged)
    Q_PROPERTY(QString stationName READ stationName NOTIFY stationNameChanged)
    Q_PROPERTY(QString radioText READ radioText NOTIFY radioTextChanged)
    Q_PROPERTY(bool alternativeFrequenciesEnabled READ isAlternativeFrequenciesEnabled
               WRITE setAlternativeFrequenciesEnabled NOTIFY alternativeFrequenciesEnabledChanged)
    Q_ENUMS(Error)
    Q_ENUMS(ProgramType)

    Q_INTERFACES(QMediaBindableInterface)

public:
    enum Error { NoError, ResourceError, OpenError, OutOfRangeError };

    enum ProgramType { Undefined = 0, News, CurrentAffairs, Information,
        Sport, Education, Drama, Culture, Science, Varied,
        PopMusic, RockMusic, EasyListening, LightClassical,
        SeriousClassical, OtherMusic, Weather, Finance,
        ChildrensProgrammes, SocialAffairs, Religion,
        PhoneIn, Travel, Leisure, JazzMusic, CountryMusic,
        NationalMusic, OldiesMusic, FolkMusic, Documentary,
        AlarmTest, Alarm, Talk, ClassicRock, AdultHits,
        SoftRock, Top40, Soft, Nostalgia, Classical,
        RhythmAndBlues, SoftRhythmAndBlues, Language,
        ReligiousMusic, ReligiousTalk, Personality, Public,
        College
    };

    explicit QRadioData(QMediaObject *mediaObject, QObject *parent = nullptr);
    ~QRadioData();

    QMultimedia::AvailabilityStatus availability() const;

    QMediaObject *mediaObject() const override;

    QString stationId() const;
    ProgramType programType() const;
    QString programTypeName() const;
    QString stationName() const;
    QString radioText() const;
    bool isAlternativeFrequenciesEnabled() const;

    Error error() const;
    QString errorString() const;

public Q_SLOTS:
    void setAlternativeFrequenciesEnabled(bool enabled);

Q_SIGNALS:
    void stationIdChanged(QString stationId);
    void programTypeChanged(QRadioData::ProgramType programType);
    void programTypeNameChanged(QString programTypeName);
    void stationNameChanged(QString stationName);
    void radioTextChanged(QString radioText);
    void alternativeFrequenciesEnabledChanged(bool enabled);

    void error(QRadioData::Error error);

protected:
    bool setMediaObject(QMediaObject *) override;

    QRadioDataPrivate *d_ptr;
private:

    Q_DISABLE_COPY(QRadioData)
    Q_DECLARE_PRIVATE(QRadioData)
    Q_PRIVATE_SLOT(d_func(), void _q_serviceDestroyed())
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QRadioData::Error)
Q_DECLARE_METATYPE(QRadioData::ProgramType)

Q_MEDIA_ENUM_DEBUG(QRadioData, Error)
Q_MEDIA_ENUM_DEBUG(QRadioData, ProgramType)

#endif  // QRADIOPLAYER_H
