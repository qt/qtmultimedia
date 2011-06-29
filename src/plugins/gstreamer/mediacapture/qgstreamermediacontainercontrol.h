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


#ifndef QGSTREAMERMEDIACONTAINERCONTROL_H
#define QGSTREAMERMEDIACONTAINERCONTROL_H

#include <qmediacontainercontrol.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qset.h>

#include <gst/gst.h>

QT_USE_NAMESPACE

class QGstreamerMediaContainerControl : public QMediaContainerControl
{
Q_OBJECT
public:
    QGstreamerMediaContainerControl(QObject *parent);
    virtual ~QGstreamerMediaContainerControl() {};

    virtual QStringList supportedContainers() const { return m_supportedContainers; }
    virtual QString containerMimeType() const { return m_format; }
    virtual void setContainerMimeType(const QString &formatMimeType) { m_format = formatMimeType; }

    virtual QString containerDescription(const QString &formatMimeType) const { return m_containerDescriptions.value(formatMimeType); }

    QByteArray formatElementName() const { return m_elementNames.value(containerMimeType()); }

    QSet<QString> supportedStreamTypes(const QString &container) const;

    static QSet<QString> supportedStreamTypes(GstElementFactory *factory, GstPadDirection direction);

    QString containerExtension() const;

private:
    QString m_format;
    QStringList m_supportedContainers;
    QMap<QString,QByteArray> m_elementNames;
    QMap<QString, QString> m_containerDescriptions;
    QMap<QString, QString> m_containerExtensions;
    QMap<QString, QSet<QString> > m_streamTypes;
};

#endif // QGSTREAMERMEDIACONTAINERCONTROL_H
