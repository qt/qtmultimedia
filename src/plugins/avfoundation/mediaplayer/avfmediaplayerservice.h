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

#ifndef AVFMEDIAPLAYERSERVICE_H
#define AVFMEDIAPLAYERSERVICE_H

#include <QtMultimedia/QMediaService>

QT_BEGIN_NAMESPACE

class AVFMediaPlayerSession;
class AVFMediaPlayerControl;
class AVFMediaPlayerMetaDataControl;
class AVFVideoOutput;

class AVFMediaPlayerService : public QMediaService
{
public:
    explicit AVFMediaPlayerService(QObject *parent = nullptr);
    ~AVFMediaPlayerService();

    QMediaControl* requestControl(const char *name) override;
    void releaseControl(QMediaControl *control) override;

private:
    AVFMediaPlayerSession *m_session;
    AVFMediaPlayerControl *m_control;
    QMediaControl *m_videoOutput;
    AVFMediaPlayerMetaDataControl *m_playerMetaDataControl;
};

QT_END_NAMESPACE

#endif // AVFMEDIAPLAYERSERVICE_H
