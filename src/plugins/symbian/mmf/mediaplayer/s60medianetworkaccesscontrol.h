/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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

#ifndef S60MEDIANETWORKACCESSCONTROL_H_
#define S60MEDIANETWORKACCESSCONTROL_H_


#include <QtCore/qobject.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>
#include <qmetaobject.h>
#include <QtNetwork/qnetworkconfiguration.h>
#include <commdbconnpref.h>
#include <commdb.h>
#include <mmf/common/mmfcontrollerframeworkbase.h>
#include <qmedianetworkaccesscontrol.h>
#include "s60mediaplayercontrol.h"

QT_BEGIN_NAMESPACE
class QMediaPlayerControl;
class QMediaNetworkAccessControl;
class QNetworkConfiguration;
QT_END_NAMESPACE

class S60MediaNetworkAccessControl : public QMediaNetworkAccessControl
{
    Q_OBJECT

public:

    S60MediaNetworkAccessControl(QObject *parent = 0);
    ~S60MediaNetworkAccessControl();

    virtual void setConfigurations(const QList<QNetworkConfiguration> &configurations);
    virtual QNetworkConfiguration currentConfiguration() const;
    int accessPointId();
    TBool isLastAccessPoint();
    void resetIndex();

public Q_SLOTS:
    void accessPointChanged(int);

private:
    void retriveAccesspointIDL(const QList<QNetworkConfiguration> &);
    QList<int> m_IapIdList;
    QList<QNetworkConfiguration> m_NetworkObjectList;
    QNetworkConfiguration m_NetworkObject;
    int m_iapId;
    int m_currentIndex;
};
#endif /* S60MEDIANETWORKACCESSCONTROL_H_ */
