/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Mobility Components.
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

    QVariant metaData(const QString &key) const;
    QStringList availableMetaData() const;

    void updateSource(IMFPresentationDescriptor* sourcePD, IMFMediaSource* mediaSource);

private:
    QVariant convertValue(const PROPVARIANT& var) const;
    IPropertyStore  *m_content;  //for Windows7
    IMFMetadata        *m_metaData; //for Vista

    QStringList m_availableMetaDatas;
    QList<PROPERTYKEY> m_commonKeys; //for Windows7
    QStringList        m_commonNames; //for Vista
};

#endif
