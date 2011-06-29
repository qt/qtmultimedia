/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "DebugMacros.h"

#include "s60mediametadataprovider.h"
#include "s60mediaplayercontrol.h"
#include "s60mediaplayersession.h"
#include <QtCore/qdebug.h>

/*!
 * Typecasts the \a control object to S60MediaPlayerControl object.
*/
S60MediaMetaDataProvider::S60MediaMetaDataProvider(QObject *control, QObject *parent)
    : QMetaDataReaderControl(parent)
    , m_control(NULL)
{
    DP0("S60MediaMetaDataProvider::S60MediaMetaDataProvider +++");

    m_control = qobject_cast<S60MediaPlayerControl*>(control);

    DP0("S60MediaMetaDataProvider::S60MediaMetaDataProvider ---");
}

/*!
 * Destructor
*/

S60MediaMetaDataProvider::~S60MediaMetaDataProvider()
{
    DP0("S60MediaMetaDataProvider::~S60MediaMetaDataProvider +++");
    DP0("S60MediaMetaDataProvider::~S60MediaMetaDataProvider ---");
}

/*!
 * Returns TRUE if MetaData is Available or else FALSE.
*/

bool S60MediaMetaDataProvider::isMetaDataAvailable() const
{
    DP0("S60MediaMetaDataProvider::isMetaDataAvailable");

    if (m_control->session())
       return m_control->session()->isMetadataAvailable();
    return false;
}

/*!
 * Always returns FLASE.
*/
bool S60MediaMetaDataProvider::isWritable() const
{
    DP0("S60MediaMetaDataProvider::isWritable");

    return false;
}

/*!
 * Returns when \a key meta data is found in metaData.
*/

QVariant S60MediaMetaDataProvider::metaData(QtMultimediaKit::MetaData key) const
{
    DP0("S60MediaMetaDataProvider::metaData");

    if (m_control->session())
        return m_control->session()->metaData(key);
    return QVariant();
}

/*!
 * Returns available metaData.
*/

QList<QtMultimediaKit::MetaData> S60MediaMetaDataProvider::availableMetaData() const
{
    DP0("S60MediaMetaDataProvider::availableMetaData");

    if (m_control->session())
        return m_control->session()->availableMetaData();
    return QList<QtMultimediaKit::MetaData>();
}

/*!
 * Returns when \a key string is found in extended metaData.
*/

QVariant S60MediaMetaDataProvider::extendedMetaData(const QString &key) const
{
    DP0("S60MediaMetaDataProvider::extendedMetaData");

    if (m_control->session())
        return m_control->session()->metaData(key);
    return QVariant();
}

/*!
 * Returns available Extended MetaData.
*/

QStringList S60MediaMetaDataProvider::availableExtendedMetaData() const
{
    DP0("S60MediaMetaDataProvider::availableExtendedMetaData");

    if (m_control->session())
        return m_control->session()->availableExtendedMetaData();
    return QStringList();
}
