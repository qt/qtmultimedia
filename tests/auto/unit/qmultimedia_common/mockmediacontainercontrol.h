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

#ifndef MOCKMEDIACONTAINERCONTROL_H
#define MOCKMEDIACONTAINERCONTROL_H

#include <QObject>
#include "qmediacontainercontrol.h"
#include <QMap>
#include <QString>
#include <QStringList>

QT_USE_NAMESPACE
class MockMediaContainerControl : public QMediaContainerControl
{
    Q_OBJECT
public:
    MockMediaContainerControl(QObject *parent):
        QMediaContainerControl(parent)
    {
        m_supportedContainers.append("wav");
        m_supportedContainers.append("mp3");
        m_supportedContainers.append("mov");

        m_descriptions.insert("wav", "WAV format");
        m_descriptions.insert("mp3", "MP3 format");
        m_descriptions.insert("mov", "MOV format");
    }

    virtual ~MockMediaContainerControl() {};

    QStringList supportedContainers() const
    {
        return m_supportedContainers;
    }

    QString containerMimeType() const
    {
        return m_format;
    }

    void setContainerMimeType(const QString &formatMimeType)
    {
        if (m_supportedContainers.contains(formatMimeType))
            m_format = formatMimeType;
    }

    QString containerDescription(const QString &formatMimeType) const
    {
        return m_descriptions.value(formatMimeType);
    }

private:
    QStringList m_supportedContainers;
    QMap<QString, QString> m_descriptions;
    QString m_format;
};

#endif // MOCKMEDIACONTAINERCONTROL_H
