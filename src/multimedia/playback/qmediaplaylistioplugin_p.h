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

#ifndef QMEDIAPLAYLISTIOPLUGIN_P_H
#define QMEDIAPLAYLISTIOPLUGIN_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qplugin.h>

#include <qtmultimediaglobal.h>

#include "qmediacontent.h"

QT_BEGIN_NAMESPACE

class QString;
class QUrl;
class QByteArray;
class QIODevice;
class QStringList;

class Q_MULTIMEDIA_EXPORT QMediaPlaylistReader
{
public:
    virtual ~QMediaPlaylistReader();

    virtual bool atEnd() const = 0;
    virtual QMediaContent readItem() = 0;
    virtual void close() = 0;
};

class Q_MULTIMEDIA_EXPORT QMediaPlaylistWriter
{
public:
    virtual ~QMediaPlaylistWriter();

    virtual bool writeItem(const QMediaContent &content) = 0;
    virtual void close() = 0;
};

struct Q_MULTIMEDIA_EXPORT QMediaPlaylistIOInterface
{
    virtual bool canRead(QIODevice *device, const QByteArray &format = QByteArray() ) const = 0;
    virtual bool canRead(const QUrl& location, const QByteArray &format = QByteArray()) const = 0;

    virtual bool canWrite(QIODevice *device, const QByteArray &format) const = 0;

    virtual QMediaPlaylistReader *createReader(QIODevice *device, const QByteArray &format = QByteArray()) = 0;
    virtual QMediaPlaylistReader *createReader(const QUrl& location, const QByteArray &format = QByteArray()) = 0;

    virtual QMediaPlaylistWriter *createWriter(QIODevice *device, const QByteArray &format) = 0;
};

#define QMediaPlaylistIOInterface_iid "org.qt-project.qt.mediaplaylistio/5.0"
Q_DECLARE_INTERFACE(QMediaPlaylistIOInterface, QMediaPlaylistIOInterface_iid);

class Q_MULTIMEDIA_EXPORT QMediaPlaylistIOPlugin : public QObject, public QMediaPlaylistIOInterface
{
Q_OBJECT
Q_INTERFACES(QMediaPlaylistIOInterface)
public:
    explicit QMediaPlaylistIOPlugin(QObject *parent = nullptr);
    ~QMediaPlaylistIOPlugin();

    bool canRead(QIODevice *device, const QByteArray &format = QByteArray() ) const override = 0;
    bool canRead(const QUrl& location, const QByteArray &format = QByteArray()) const override = 0;

    bool canWrite(QIODevice *device, const QByteArray &format) const override = 0;

    QMediaPlaylistReader *createReader(QIODevice *device, const QByteArray &format = QByteArray()) override = 0;
    QMediaPlaylistReader *createReader(const QUrl& location, const QByteArray &format = QByteArray()) override = 0;

    QMediaPlaylistWriter *createWriter(QIODevice *device, const QByteArray &format) override = 0;
};

QT_END_NAMESPACE


#endif // QMEDIAPLAYLISTIOPLUGIN_P_H
