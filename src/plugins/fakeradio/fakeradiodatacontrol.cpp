/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "fakeradiodatacontrol.h"
#include "fakeradioservice.h"

#include <QtCore/qdebug.h>

FakeRadioDataControl::FakeRadioDataControl(QObject *parent)
    :QRadioDataControl(parent)
{
    initializeProgramTypeMapping();

    m_rdsTimer = new QTimer(this);
    connect(m_rdsTimer,SIGNAL(timeout()),this,SLOT(rdsUpdate()));
    m_rdsTimer->start(5000);
    rdsUpdate();

    qsrand(QTime::currentTime().msec());
}

FakeRadioDataControl::~FakeRadioDataControl()
{
}

bool FakeRadioDataControl::isAvailable() const
{
    return true;
}

QtMultimedia::AvailabilityError FakeRadioDataControl::availabilityError() const
{
    return QtMultimedia::NoError;
}

QString FakeRadioDataControl::stationId() const
{
    return "12345678";
}

QRadioData::ProgramType FakeRadioDataControl::programType() const
{
    return QRadioData::Drama;
}

QString FakeRadioDataControl::programTypeName() const
{
    return "Cycling";
}

QString FakeRadioDataControl::stationName() const
{
    return "Fake FM";
}

void FakeRadioDataControl::rdsUpdate()
{
    static int index = 0;
    QString rdsStrings[] = {
        "This is radio Fake FM",
        "There is nothing to listen to here",
        "Please remain calm" };
    setradioText(rdsStrings[index%3]);
    index++;
}

void FakeRadioDataControl::setradioText(QString text)
{
    m_radioText = text;
    emit radioTextChanged(m_radioText);
}

QString FakeRadioDataControl::radioText() const
{
    return m_radioText;
}

void FakeRadioDataControl::setAlternativeFrequenciesEnabled(bool enabled)
{
    m_alternativeFrequenciesEnabled = enabled;
}

bool FakeRadioDataControl::isAlternativeFrequenciesEnabled() const
{
    return m_alternativeFrequenciesEnabled;
}

QRadioData::Error FakeRadioDataControl::error() const
{
    return QRadioData::NoError;
}

QString FakeRadioDataControl::errorString() const
{
    return QString();
}

void FakeRadioDataControl::initializeProgramTypeMapping()
{
    m_programTypeMapRDS[0] = QRadioData::Undefined;
    m_programTypeMapRDS[1] = QRadioData::News;
    m_programTypeMapRDS[2] = QRadioData::CurrentAffairs;
    m_programTypeMapRDS[3] = QRadioData::Information;
    m_programTypeMapRDS[4] = QRadioData::Sport;
    m_programTypeMapRDS[5] = QRadioData::Education;
    m_programTypeMapRDS[6] = QRadioData::Drama;
    m_programTypeMapRDS[7] = QRadioData::Culture;
    m_programTypeMapRDS[8] = QRadioData::Science;
    m_programTypeMapRDS[9] = QRadioData::Varied;
    m_programTypeMapRDS[10] = QRadioData::PopMusic;
    m_programTypeMapRDS[11] = QRadioData::RockMusic;
    m_programTypeMapRDS[12] = QRadioData::EasyListening;
    m_programTypeMapRDS[13] = QRadioData::LightClassical;
    m_programTypeMapRDS[14] = QRadioData::SeriousClassical;
    m_programTypeMapRDS[15] = QRadioData::OtherMusic;
    m_programTypeMapRDS[16] = QRadioData::Weather;
    m_programTypeMapRDS[17] = QRadioData::Finance;
    m_programTypeMapRDS[18] = QRadioData::ChildrensProgrammes;
    m_programTypeMapRDS[19] = QRadioData::SocialAffairs;
    m_programTypeMapRDS[20] = QRadioData::Religion;
    m_programTypeMapRDS[21] = QRadioData::PhoneIn;
    m_programTypeMapRDS[22] = QRadioData::Travel;
    m_programTypeMapRDS[23] = QRadioData::Leisure;
    m_programTypeMapRDS[24] = QRadioData::JazzMusic;
    m_programTypeMapRDS[25] = QRadioData::CountryMusic;
    m_programTypeMapRDS[26] = QRadioData::NationalMusic;
    m_programTypeMapRDS[27] = QRadioData::OldiesMusic;
    m_programTypeMapRDS[28] = QRadioData::FolkMusic;
    m_programTypeMapRDS[29] = QRadioData::Documentary;
    m_programTypeMapRDS[30] = QRadioData::AlarmTest;
    m_programTypeMapRDS[31] = QRadioData::Alarm;

    m_programTypeMapRBDS[0] = QRadioData::Undefined,
    m_programTypeMapRBDS[1] = QRadioData::News;
    m_programTypeMapRBDS[2] = QRadioData::Information;
    m_programTypeMapRBDS[3] = QRadioData::Sport;
    m_programTypeMapRBDS[4] = QRadioData::Talk;
    m_programTypeMapRBDS[5] = QRadioData::RockMusic;
    m_programTypeMapRBDS[6] = QRadioData::ClassicRock;
    m_programTypeMapRBDS[7] = QRadioData::AdultHits;
    m_programTypeMapRBDS[8] = QRadioData::SoftRock;
    m_programTypeMapRBDS[9] = QRadioData::Top40;
    m_programTypeMapRBDS[10] = QRadioData::CountryMusic;
    m_programTypeMapRBDS[11] = QRadioData::OldiesMusic;
    m_programTypeMapRBDS[12] = QRadioData::Soft;
    m_programTypeMapRBDS[13] = QRadioData::Nostalgia;
    m_programTypeMapRBDS[14] = QRadioData::JazzMusic;
    m_programTypeMapRBDS[15] = QRadioData::Classical;
    m_programTypeMapRBDS[16] = QRadioData::RhythmAndBlues;
    m_programTypeMapRBDS[17] = QRadioData::SoftRhythmAndBlues;
    m_programTypeMapRBDS[18] = QRadioData::Language;
    m_programTypeMapRBDS[19] = QRadioData::ReligiousMusic;
    m_programTypeMapRBDS[20] = QRadioData::ReligiousTalk;
    m_programTypeMapRBDS[21] = QRadioData::Personality;
    m_programTypeMapRBDS[22] = QRadioData::Public;
    m_programTypeMapRBDS[23] = QRadioData::College;
    m_programTypeMapRBDS[24] = QRadioData::Undefined;
    m_programTypeMapRBDS[25] = QRadioData::Undefined;
    m_programTypeMapRBDS[26] = QRadioData::Undefined;
    m_programTypeMapRBDS[27] = QRadioData::Undefined;
    m_programTypeMapRBDS[28] = QRadioData::Undefined;
    m_programTypeMapRBDS[29] = QRadioData::Weather;
    m_programTypeMapRBDS[30] = QRadioData::AlarmTest;
    m_programTypeMapRBDS[31] = QRadioData::Alarm;
}

bool FakeRadioDataControl::usingRBDS()
{
    switch ( QLocale::system().country() )
    {
        case QLocale::Canada:
        case QLocale::Mexico:
        case QLocale::UnitedStates:
            return true;

        default:
            return false;
    }
    return false;
}

QRadioData::ProgramType FakeRadioDataControl::fromRawProgramType(int rawProgramType)
{
    if ( usingRBDS() )
        return m_programTypeMapRBDS.value(rawProgramType, QRadioData::Undefined);

    return m_programTypeMapRDS.value(rawProgramType, QRadioData::Undefined);
}
