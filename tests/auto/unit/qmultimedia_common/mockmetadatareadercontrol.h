/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef MOCKMETADATAREADERCONTROL_H
#define MOCKMETADATAREADERCONTROL_H

#include "qmetadatareadercontrol.h"

class MockMetaDataReaderControl : public QMetaDataReaderControl
{
    Q_OBJECT
public:
    MockMetaDataReaderControl(QObject *parent = 0)
        : QMetaDataReaderControl(parent)
        , m_available(false)
    {
    }

    bool isMetaDataAvailable() const
    {
        return m_available;
    }
    void setMetaDataAvailable(bool available)
    {
        if (m_available != available)
            emit metaDataAvailableChanged(m_available = available);
    }
    QStringList availableMetaData() const
    {
        return m_data.keys();
    }

    QVariant metaData(const QString &key) const
    {
        return m_data.value(key);
    }

    using QMetaDataReaderControl::metaDataChanged;

    void populateMetaData()
    {
        m_available = true;
    }

    bool m_available;
    QMap<QString, QVariant> m_data;
};

#endif // MOCKMETADATAREADERCONTROL_H
