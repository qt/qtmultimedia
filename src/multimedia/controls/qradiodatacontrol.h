/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QRADIODATACONTROL_H
#define QRADIODATACONTROL_H

#include "qmediacontrol.h"
#include "qradiodata.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Multimedia)

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QRadioDataControl : public QMediaControl
{
    Q_OBJECT

public:
    ~QRadioDataControl();

    virtual QString stationId() const = 0;
    virtual QRadioData::ProgramType programType() const = 0;
    virtual QString programTypeName() const = 0;
    virtual QString stationName() const = 0;
    virtual QString radioText() const = 0;
    virtual void setAlternativeFrequenciesEnabled(bool enabled) = 0;
    virtual bool isAlternativeFrequenciesEnabled() const = 0;

    virtual QRadioData::Error error() const = 0;
    virtual QString errorString() const = 0;

Q_SIGNALS:
    void stationIdChanged(QString stationId);
    void programTypeChanged(QRadioData::ProgramType programType);
    void programTypeNameChanged(QString programTypeName);
    void stationNameChanged(QString stationName);
    void radioTextChanged(QString radioText);
    void alternativeFrequenciesEnabledChanged(bool enabled);
    void error(QRadioData::Error err);

protected:
    QRadioDataControl(QObject *parent = 0);
};

#define QRadioDataControl_iid "org.qt-project.qt.radiodatacontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QRadioDataControl, QRadioDataControl_iid)

QT_END_NAMESPACE

QT_END_HEADER


#endif  // QRADIODATACONTROL_H
