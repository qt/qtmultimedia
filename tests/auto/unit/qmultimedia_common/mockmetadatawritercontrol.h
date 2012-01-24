/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKMETADATAWRITERCONTROL_H
#define MOCKMETADATAWRITERCONTROL_H

#include <QObject>
#include <QMap>

#include "qmetadatawritercontrol.h"

class MockMetaDataWriterControl : public QMetaDataWriterControl
{
    Q_OBJECT
public:
    MockMetaDataWriterControl(QObject *parent = 0)
        : QMetaDataWriterControl(parent)
        , m_available(false)
        , m_writable(false)
    {
    }

    bool isMetaDataAvailable() const { return m_available; }
    void setMetaDataAvailable(bool available)
    {
        if (m_available != available)
            emit metaDataAvailableChanged(m_available = available);
    }
    QStringList availableMetaData() const { return m_data.keys(); }

    bool isWritable() const { return m_writable; }
    void setWritable(bool writable) { emit writableChanged(m_writable = writable); }

    QVariant metaData(const QString &key) const { return m_data.value(key); }//Getting the metadata from Multimediakit
    void setMetaData(const QString &key, const QVariant &value)
    {
        m_data.insert(key, value);
    }

    using QMetaDataWriterControl::metaDataChanged;

    void populateMetaData()
    {
        m_available = true;
    }
    void setWritable()
    {
        emit writableChanged(true);
    }
    void setMetaDataAvailable()
    {
        emit metaDataAvailableChanged(true);
    }

    bool m_available;
    bool m_writable;
    QMap<QString, QVariant> m_data;
};

#endif // MOCKMETADATAWRITERCONTROL_H
