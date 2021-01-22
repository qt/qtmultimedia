/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QMEDIACONTENT_H
#define QMEDIACONTENT_H

#include <QtCore/qmetatype.h>
#include <QtCore/qshareddata.h>

#include <QtMultimedia/qmediaresource.h>

#include <QtMultimedia/qtmultimediaglobal.h>

QT_BEGIN_NAMESPACE

class QMediaPlaylist;

class QMediaContentPrivate;
class Q_MULTIMEDIA_EXPORT QMediaContent
{
public:
    QMediaContent();
    QMediaContent(const QUrl &contentUrl);
    QMediaContent(const QNetworkRequest &contentRequest);
#if QT_DEPRECATED_SINCE(6, 0)
    QT_DEPRECATED QMediaContent(const QMediaResource &contentResource);
    QT_DEPRECATED QMediaContent(const QMediaResourceList &resources);
#endif
    QMediaContent(const QMediaContent &other);
    QMediaContent(QMediaPlaylist *playlist, const QUrl &contentUrl = QUrl(), bool takeOwnership = false);
    ~QMediaContent();

    QMediaContent& operator=(const QMediaContent &other);

    bool operator==(const QMediaContent &other) const;
    bool operator!=(const QMediaContent &other) const;

    bool isNull() const;
    QNetworkRequest request() const;

#if QT_DEPRECATED_SINCE(6, 0)
    QT_DEPRECATED_X("Use QMediaContent::request().url()") QUrl canonicalUrl() const;
    QT_DEPRECATED_X("Use QMediaContent::request()") QNetworkRequest canonicalRequest() const;
    QT_DEPRECATED QMediaResource canonicalResource() const;
    QT_DEPRECATED QMediaResourceList resources() const;
#endif

    QMediaPlaylist *playlist() const;
private:
    QSharedDataPointer<QMediaContentPrivate> d;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QMediaContent)

#endif  // QMEDIACONTENT_H
