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

#ifndef QMEDIAPLAYLISTPROVIDER_P_H
#define QMEDIAPLAYLISTPROVIDER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmediacontent.h"
#include "qmediaplaylist.h"

QT_BEGIN_NAMESPACE


class QMediaPlaylistProviderPrivate
{
public:
    QMediaPlaylistProviderPrivate()
    {}
    virtual ~QMediaPlaylistProviderPrivate()
    {}
};

class Q_MULTIMEDIA_EXPORT QMediaPlaylistProvider : public QObject
{
Q_OBJECT
public:
    QMediaPlaylistProvider(QObject *parent = nullptr);
    virtual ~QMediaPlaylistProvider();

    virtual bool load(const QNetworkRequest &request, const char *format = nullptr);
    virtual bool load(QIODevice *device, const char *format = nullptr);
    virtual bool save(const QUrl &location, const char *format = nullptr);
    virtual bool save(QIODevice * device, const char *format);

    virtual int mediaCount() const = 0;
    virtual QMediaContent media(int index) const = 0;

    virtual bool isReadOnly() const;

    virtual bool addMedia(const QMediaContent &content);
    virtual bool addMedia(const QList<QMediaContent> &contentList);
    virtual bool insertMedia(int index, const QMediaContent &content);
    virtual bool insertMedia(int index, const QList<QMediaContent> &content);
    virtual bool moveMedia(int from, int to);
    virtual bool removeMedia(int pos);
    virtual bool removeMedia(int start, int end);
    virtual bool clear();

public Q_SLOTS:
    virtual void shuffle();

Q_SIGNALS:
    void mediaAboutToBeInserted(int start, int end);
    void mediaInserted(int start, int end);

    void mediaAboutToBeRemoved(int start, int end);
    void mediaRemoved(int start, int end);

    void mediaChanged(int start, int end);

    void loaded();
    void loadFailed(QMediaPlaylist::Error, const QString& errorMessage);

protected:
    QMediaPlaylistProviderPrivate *d_ptr;
    QMediaPlaylistProvider(QMediaPlaylistProviderPrivate &dd, QObject *parent);

private:
    Q_DECLARE_PRIVATE(QMediaPlaylistProvider)
};

QT_END_NAMESPACE



#endif // QMEDIAPLAYLISTSOURCE_P_H
