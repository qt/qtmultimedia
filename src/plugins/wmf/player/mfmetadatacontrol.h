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

#ifndef MFMETADATACONTROL_H
#define MFMETADATACONTROL_H

#include <qmetadatareadercontrol.h>
#include "Mfidl.h"

QT_USE_NAMESPACE

class MFMetaDataControl : public QMetaDataReaderControl
{
    Q_OBJECT
public:
    MFMetaDataControl(QObject *parent = 0);
    ~MFMetaDataControl();

    bool isMetaDataAvailable() const;

    QVariant metaData(QtMultimedia::MetaData key) const;
    QList<QtMultimedia::MetaData> availableMetaData() const;

    QVariant extendedMetaData(const QString &key) const;
    QStringList availableExtendedMetaData() const;

    void updateSource(IMFPresentationDescriptor* sourcePD, IMFMediaSource* mediaSource);

private:
    QVariant convertValue(const PROPVARIANT& var) const;
    IPropertyStore  *m_content;  //for Windows7
    IMFMetadata        *m_metaData; //for Vista

    QList<QtMultimedia::MetaData> m_availableMetaDatas;
    QList<PROPERTYKEY> m_commonKeys; //for Windows7
    QStringList        m_commonNames; //for Vista

    QStringList m_extendedMetaDatas;
    QList<PROPERTYKEY> m_extendedKeys; //for Windows7
};

#endif
