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

#include "qdeclarativeradiodata_p.h"

QT_BEGIN_NAMESPACE

QDeclarativeRadioData::QDeclarativeRadioData(QObject *parent) :
    QObject(parent),
    m_radioData(0)
{
    m_radioData = new QRadioData(this);

    connect(m_radioData, SIGNAL(programTypeChanged(QRadioData::ProgramType)), this,
                                 SLOT(_q_programTypeChanged(QRadioData::ProgramType)));

    connect(m_radioData, SIGNAL(stationIdChanged(QString)), this, SIGNAL(stationIdChanged(QString)));
    connect(m_radioData, SIGNAL(programTypeNameChanged(QString)), this, SIGNAL(programTypeNameChanged(QString)));
    connect(m_radioData, SIGNAL(stationNameChanged(QString)), this, SIGNAL(stationNameChanged(QString)));
    connect(m_radioData, SIGNAL(radioTextChanged(QString)), this, SIGNAL(radioTextChanged(QString)));
    connect(m_radioData, SIGNAL(alternativeFrequenciesEnabledChanged(bool)), this,
                         SIGNAL(alternativeFrequenciesEnabledChanged(bool)));

    connect(m_radioData, SIGNAL(error(QRadioData::Error)), this, SLOT(_q_error(QRadioData::Error)));
}

QDeclarativeRadioData::~QDeclarativeRadioData()
{
}

bool QDeclarativeRadioData::isAvailable() const
{
    return m_radioData->isAvailable();
}

QString QDeclarativeRadioData::stationId() const
{
    return m_radioData->stationId();
}

QDeclarativeRadioData::ProgramType QDeclarativeRadioData::programType() const
{
    return static_cast<QDeclarativeRadioData::ProgramType>(m_radioData->programType());
}

QString QDeclarativeRadioData::programTypeName() const
{
    return m_radioData->programTypeName();
}

QString QDeclarativeRadioData::stationName() const
{
    return m_radioData->stationName();
}

QString QDeclarativeRadioData::radioText() const
{
    return m_radioData->radioText();
}

bool QDeclarativeRadioData::alternativeFrequenciesEnabled() const
{
    return m_radioData->isAlternativeFrequenciesEnabled();
}

void QDeclarativeRadioData::setAlternativeFrequenciesEnabled(bool enabled)
{
    m_radioData->setAlternativeFrequenciesEnabled(enabled);
}

void QDeclarativeRadioData::_q_programTypeChanged(QRadioData::ProgramType programType)
{
    emit programTypeChanged(static_cast<QDeclarativeRadioData::ProgramType>(programType));
}

void QDeclarativeRadioData::_q_error(QRadioData::Error errorCode)
{
    emit error(static_cast<QDeclarativeRadioData::Error>(errorCode));
    emit errorChanged();
}
