/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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


#ifndef CAMERABINMEDIACONTAINERCONTROL_H
#define CAMERABINMEDIACONTAINERCONTROL_H

#include <qmediacontainercontrol.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qset.h>

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include <gst/pbutils/encoding-profile.h>

#include "qgstcodecsinfo.h"

QT_USE_NAMESPACE

class CameraBinContainer : public QMediaContainerControl
{
Q_OBJECT
public:
    CameraBinContainer(QObject *parent);
    virtual ~CameraBinContainer() {}

    virtual QStringList supportedContainers() const;
    virtual QString containerDescription(const QString &formatMimeType) const;

    virtual QString containerMimeType() const;
    virtual void setContainerMimeType(const QString &formatMimeType);

    void setActualContainer(const QString &formatMimeType);
    void resetActualContainer();

    QString suggestedFileExtension() const;

    GstEncodingContainerProfile *createProfile();

Q_SIGNALS:
    void settingsChanged();

private:
    QString m_format; // backend selected format, using m_userFormat
    QString m_userFormat;
    QMap<QString, QString> m_fileExtensions;

    QGstCodecsInfo m_supportedContainers;
};

#endif // CAMERABINMEDIACONTAINERCONTROL_H
