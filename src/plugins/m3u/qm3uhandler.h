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

#ifndef QM3UHANDLER_H
#define QM3UHANDLER_H

#include <private/qmediaplaylistioplugin_p.h>
#include <QtCore/QObject>

QT_USE_NAMESPACE

class QM3uPlaylistPlugin : public QMediaPlaylistIOPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.mediaplaylistio/5.0" FILE "m3u.json")

public:
    explicit QM3uPlaylistPlugin(QObject *parent = 0);
    virtual ~QM3uPlaylistPlugin();

    bool canRead(QIODevice *device, const QByteArray &format = QByteArray()) const override;
    bool canRead(const QUrl& location, const QByteArray &format = QByteArray()) const override;

    bool canWrite(QIODevice *device, const QByteArray &format) const override;

    QMediaPlaylistReader *createReader(QIODevice *device, const QByteArray &format = QByteArray()) override;
    QMediaPlaylistReader *createReader(const QUrl& location, const QByteArray &format = QByteArray()) override;

    QMediaPlaylistWriter *createWriter(QIODevice *device, const QByteArray &format) override;
};

#endif // QM3UHANDLER_H
