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


#ifndef QMEDIAPLAYLISTSOURCECONTROL_P_H
#define QMEDIAPLAYLISTSOURCECONTROL_P_H

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

#include <qmediacontrol.h>

QT_BEGIN_NAMESPACE


class QMediaPlaylist;

class Q_MULTIMEDIA_EXPORT QMediaPlaylistSourceControl : public QMediaControl
{
    Q_OBJECT

public:
    virtual ~QMediaPlaylistSourceControl();

    virtual QMediaPlaylist *playlist() const = 0;
    virtual void setPlaylist(QMediaPlaylist *) = 0;

Q_SIGNALS:
    void playlistChanged(QMediaPlaylist* playlist);

protected:
    QMediaPlaylistSourceControl(QObject *parent = nullptr);
};

#define QMediaPlaylistSourceControl_iid "org.qt-project.qt.mediaplaylistsourcecontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QMediaPlaylistSourceControl, QMediaPlaylistSourceControl_iid)

QT_END_NAMESPACE


#endif // QMEDIAPLAYLISTCONTROL_P_H
