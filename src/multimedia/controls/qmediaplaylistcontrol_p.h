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


#ifndef QMEDIAPLAYLISTCONTROL_P_H
#define QMEDIAPLAYLISTCONTROL_P_H

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
#include "qmediacontrol.h"
#include <private/qmediaplaylistnavigator_p.h>


QT_BEGIN_NAMESPACE


class QMediaPlaylistProvider;

class Q_MULTIMEDIA_EXPORT QMediaPlaylistControl : public QMediaControl
{
    Q_OBJECT

public:
    virtual ~QMediaPlaylistControl();

    virtual QMediaPlaylistProvider* playlistProvider() const = 0;
    virtual bool setPlaylistProvider(QMediaPlaylistProvider *playlist) = 0;

    virtual int currentIndex() const = 0;
    virtual void setCurrentIndex(int position) = 0;
    virtual int nextIndex(int steps) const = 0;
    virtual int previousIndex(int steps) const = 0;

    virtual void next() = 0;
    virtual void previous() = 0;

    virtual QMediaPlaylist::PlaybackMode playbackMode() const = 0;
    virtual void setPlaybackMode(QMediaPlaylist::PlaybackMode mode) = 0;

Q_SIGNALS:
    void playlistProviderChanged();
    void currentIndexChanged(int position);
    void currentMediaChanged(const QMediaContent&);
    void playbackModeChanged(QMediaPlaylist::PlaybackMode mode);

protected:
    QMediaPlaylistControl(QObject *parent = nullptr);
};

#define QMediaPlaylistControl_iid "org.qt-project.qt.mediaplaylistcontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QMediaPlaylistControl, QMediaPlaylistControl_iid)

QT_END_NAMESPACE


#endif // QMEDIAPLAYLISTCONTROL_P_H
