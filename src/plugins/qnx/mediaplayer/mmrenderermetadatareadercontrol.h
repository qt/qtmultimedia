/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#ifndef MMRENDERERMETADATAREADERCONTROL_H
#define MMRENDERERMETADATAREADERCONTROL_H

#include "mmrenderermetadata.h"
#include <qmetadatareadercontrol.h>

QT_BEGIN_NAMESPACE

class MmRendererMetaDataReaderControl : public QMetaDataReaderControl
{
    Q_OBJECT
public:
    explicit MmRendererMetaDataReaderControl(QObject *parent = 0);

    bool isMetaDataAvailable() const override;

    QVariant metaData(const QString &key) const override;
    QStringList availableMetaData() const override;

    void setMetaData(const MmRendererMetaData &data);

private:
    MmRendererMetaData m_metaData;
};

QT_END_NAMESPACE

#endif
