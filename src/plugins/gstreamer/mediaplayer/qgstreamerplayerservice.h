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

#ifndef QGSTREAMERPLAYERSERVICE_H
#define QGSTREAMERPLAYERSERVICE_H

#include <QtCore/qobject.h>
#include <QtCore/qiodevice.h>

#include <qmediaservice.h>

QT_BEGIN_NAMESPACE
class QMediaPlayerControl;
class QMediaPlaylist;
class QMediaPlaylistNavigator;

class QGstreamerMetaData;
class QGstreamerPlayerControl;
class QGstreamerPlayerSession;
class QGstreamerMetaDataProvider;
class QGstreamerStreamsControl;
class QGstreamerVideoRenderer;
class QGstreamerVideoWindow;
class QGstreamerVideoWidgetControl;
class QGStreamerAvailabilityControl;
class QGstreamerAudioProbeControl;
class QGstreamerVideoProbeControl;

class QGstreamerPlayerService : public QMediaService
{
    Q_OBJECT
public:
    QGstreamerPlayerService(QObject *parent = 0);
    ~QGstreamerPlayerService();

    QMediaControl *requestControl(const char *name) override;
    void releaseControl(QMediaControl *control) override;

private:
    QGstreamerPlayerControl *m_control = nullptr;
    QGstreamerPlayerSession *m_session = nullptr;
    QGstreamerMetaDataProvider *m_metaData = nullptr;
    QGstreamerStreamsControl *m_streamsControl = nullptr;
    QGStreamerAvailabilityControl *m_availabilityControl = nullptr;

    QGstreamerAudioProbeControl *m_audioProbeControl = nullptr;
    QGstreamerVideoProbeControl *m_videoProbeControl = nullptr;

    QMediaControl *m_videoOutput = nullptr;
    QMediaControl *m_videoRenderer = nullptr;
    QGstreamerVideoWindow *m_videoWindow = nullptr;
#if defined(HAVE_WIDGETS)
    QGstreamerVideoWidgetControl *m_videoWidget = nullptr;
#endif

    void increaseVideoRef();
    void decreaseVideoRef();
    int m_videoReferenceCount = 0;
};

QT_END_NAMESPACE

#endif
