/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>

#include "qmediacontent.h"

QT_BEGIN_NAMESPACE

namespace
{
    class QMediaContentPrivateRegisterMetaTypes
    {
    public:
        QMediaContentPrivateRegisterMetaTypes()
        {
            qRegisterMetaType<QMediaContent>();
        }
    } _registerMetaTypes;
}


class QMediaContentPrivate : public QSharedData
{
public:
    QMediaContentPrivate() {}
    QMediaContentPrivate(const QMediaResourceList &r):
        resources(r) {}

    QMediaContentPrivate(const QMediaContentPrivate &other):
        QSharedData(other),
        resources(other.resources)
    {}

    bool operator ==(const QMediaContentPrivate &other) const
    {
        return resources == other.resources;
    }

    QMediaResourceList  resources;
private:
    QMediaContentPrivate& operator=(const QMediaContentPrivate &other);
};


/*!
    \class QMediaContent

    \brief The QMediaContent class provides access to the resources relating to a media content.

    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_playback

    QMediaContent is used within the multimedia framework as the logical handle
    to media content.  A QMediaContent object is composed of one or more
    \l {QMediaResource}s where each resource provides the URL and format
    information of a different encoding of the content.

    A non-null QMediaContent will always have a primary or canonical reference to
    the content available through the canonicalUrl() or canonicalResource()
    methods, any additional resources are optional.
*/


/*!
    Constructs a null QMediaContent.
*/

QMediaContent::QMediaContent()
{
}

/*!
    Constructs a media content with \a url providing a reference to the content.
*/

QMediaContent::QMediaContent(const QUrl &url):
    d(new QMediaContentPrivate)
{
    d->resources << QMediaResource(url);
}

/*!
    Constructs a media content with \a request providing a reference to the content.

    This constructor can be used to reference media content via network protocols such as HTTP.
    This may include additional information required to obtain the resource, such as Cookies or HTTP headers.
*/

QMediaContent::QMediaContent(const QNetworkRequest &request):
    d(new QMediaContentPrivate)
{
    d->resources << QMediaResource(request);
}

/*!
    Constructs a media content with \a resource providing a reference to the content.
*/

QMediaContent::QMediaContent(const QMediaResource &resource):
    d(new QMediaContentPrivate)
{
    d->resources << resource;
}

/*!
    Constructs a media content with \a resources providing a reference to the content.
*/

QMediaContent::QMediaContent(const QMediaResourceList &resources):
    d(new QMediaContentPrivate(resources))
{
}

/*!
    Constructs a copy of the media content \a other.
*/

QMediaContent::QMediaContent(const QMediaContent &other):
    d(other.d)
{
}

/*!
    Destroys the media content object.
*/

QMediaContent::~QMediaContent()
{
}

/*!
    Assigns the value of \a other to this media content.
*/

QMediaContent& QMediaContent::operator=(const QMediaContent &other)
{
    d = other.d;
    return *this;
}

/*!
    Returns true if \a other is equivalent to this media content; false otherwise.
*/

bool QMediaContent::operator==(const QMediaContent &other) const
{
    return (d.constData() == 0 && other.d.constData() == 0) ||
            (d.constData() != 0 && other.d.constData() != 0 &&
             *d.constData() == *other.d.constData());
}

/*!
    Returns true if \a other is not equivalent to this media content; false otherwise.
*/

bool QMediaContent::operator!=(const QMediaContent &other) const
{
    return !(*this == other);
}

/*!
    Returns true if this media content is null (uninitialized); false otherwise.
*/

bool QMediaContent::isNull() const
{
    return d.constData() == 0;
}

/*!
    Returns a QUrl that represents that canonical resource for this media content.
*/

QUrl QMediaContent::canonicalUrl() const
{
    return canonicalResource().url();
}

/*!
    Returns a QNetworkRequest that represents that canonical resource for this media content.
*/

QNetworkRequest QMediaContent::canonicalRequest() const
{
    return canonicalResource().request();
}

/*!
    Returns a QMediaResource that represents that canonical resource for this media content.
*/

QMediaResource QMediaContent::canonicalResource() const
{
    return d.constData() != 0
            ?  d->resources.value(0)
            : QMediaResource();
}

/*!
    Returns a list of alternative resources for this media content.  The first item in this list
    is always the canonical resource.
*/

QMediaResourceList QMediaContent::resources() const
{
    return d.constData() != 0
            ? d->resources
            : QMediaResourceList();
}

QT_END_NAMESPACE

