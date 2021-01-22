/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Mobility Components.
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

#ifndef MFPLAYERSERVICE_H
#define MFPLAYERSERVICE_H

#include <mfapi.h>
#include <mfidl.h>

#include "qmediaplayer.h"
#include "qmediaresource.h"
#include "qmediaservice.h"
#include "qmediatimerange.h"

QT_BEGIN_NAMESPACE
class QMediaContent;
QT_END_NAMESPACE

QT_USE_NAMESPACE

class MFEvrVideoWindowControl;
class MFAudioEndpointControl;
class MFVideoRendererControl;
class MFPlayerControl;
class MFMetaDataControl;
class MFPlayerSession;

class MFPlayerService : public QMediaService
{
    Q_OBJECT
public:
    MFPlayerService(QObject *parent = 0);
    ~MFPlayerService();

    QMediaControl* requestControl(const char *name);
    void releaseControl(QMediaControl *control);

    MFAudioEndpointControl* audioEndpointControl() const;
    MFVideoRendererControl* videoRendererControl() const;
    MFEvrVideoWindowControl* videoWindowControl() const;
    MFMetaDataControl* metaDataControl() const;

private:
    MFPlayerSession *m_session;
    MFVideoRendererControl *m_videoRendererControl;
    MFAudioEndpointControl *m_audioEndpointControl;
    MFEvrVideoWindowControl *m_videoWindowControl;
    MFPlayerControl        *m_player;
    MFMetaDataControl      *m_metaDataControl;
};

#endif
