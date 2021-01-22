/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#ifndef MMRENDERERMEDIAPLAYERSERVICE_H
#define MMRENDERERMEDIAPLAYERSERVICE_H

#include <qmediaservice.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class MmRendererAudioRoleControl;
class MmRendererCustomAudioRoleControl;
class MmRendererMediaPlayerControl;
class MmRendererMetaDataReaderControl;
class MmRendererPlayerVideoRendererControl;
class MmRendererVideoWindowControl;

class MmRendererMediaPlayerService : public QMediaService
{
    Q_OBJECT
public:
    explicit MmRendererMediaPlayerService(QObject *parent = 0);
    ~MmRendererMediaPlayerService();

    QMediaControl *requestControl(const char *name) override;
    void releaseControl(QMediaControl *control) override;

private:
    void updateControls();

    QPointer<MmRendererPlayerVideoRendererControl> m_videoRendererControl;
    QPointer<MmRendererVideoWindowControl> m_videoWindowControl;
    QPointer<MmRendererMediaPlayerControl> m_mediaPlayerControl;
    QPointer<MmRendererMetaDataReaderControl> m_metaDataReaderControl;
    QPointer<MmRendererAudioRoleControl> m_audioRoleControl;
    QPointer<MmRendererCustomAudioRoleControl> m_customAudioRoleControl;

    bool m_appHasDrmPermission : 1;
    bool m_appHasDrmPermissionChecked : 1;
};

QT_END_NAMESPACE

#endif
