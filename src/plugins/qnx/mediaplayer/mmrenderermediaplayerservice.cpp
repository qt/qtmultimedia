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
#include "mmrenderermediaplayerservice.h"

#include "mmrendereraudiorolecontrol.h"
#include "mmrenderercustomaudiorolecontrol.h"
#include "mmrenderermediaplayercontrol.h"
#include "mmrenderermetadatareadercontrol.h"
#include "mmrendererplayervideorenderercontrol.h"
#include "mmrendererutil.h"
#include "mmrenderervideowindowcontrol.h"

#include "mmreventmediaplayercontrol.h"

QT_BEGIN_NAMESPACE

MmRendererMediaPlayerService::MmRendererMediaPlayerService(QObject *parent)
    : QMediaService(parent),
      m_videoRendererControl(0),
      m_videoWindowControl(0),
      m_mediaPlayerControl(0),
      m_metaDataReaderControl(0),
      m_appHasDrmPermission(false),
      m_appHasDrmPermissionChecked(false)
{
}

MmRendererMediaPlayerService::~MmRendererMediaPlayerService()
{
    // Someone should have called releaseControl(), but better be safe
    delete m_videoRendererControl;
    delete m_videoWindowControl;
    delete m_mediaPlayerControl;
    delete m_metaDataReaderControl;
    delete m_audioRoleControl;
    delete m_customAudioRoleControl;
}

QMediaControl *MmRendererMediaPlayerService::requestControl(const char *name)
{
    if (qstrcmp(name, QMediaPlayerControl_iid) == 0) {
        if (!m_mediaPlayerControl) {
            m_mediaPlayerControl = new MmrEventMediaPlayerControl;
            updateControls();
        }
        return m_mediaPlayerControl;
    } else if (qstrcmp(name, QMetaDataReaderControl_iid) == 0) {
        if (!m_metaDataReaderControl) {
            m_metaDataReaderControl = new MmRendererMetaDataReaderControl();
            updateControls();
        }
        return m_metaDataReaderControl;
    } else if (qstrcmp(name, QAudioRoleControl_iid) == 0) {
        if (!m_audioRoleControl) {
            m_audioRoleControl = new MmRendererAudioRoleControl();
            updateControls();
        }
        return m_audioRoleControl;
    } else if (qstrcmp(name, QCustomAudioRoleControl_iid) == 0) {
        if (!m_customAudioRoleControl) {
            m_customAudioRoleControl = new MmRendererCustomAudioRoleControl();
            updateControls();
        }
        return m_customAudioRoleControl;
    } else if (qstrcmp(name, QVideoRendererControl_iid) == 0) {
        if (!m_appHasDrmPermissionChecked) {
            m_appHasDrmPermission = checkForDrmPermission();
            m_appHasDrmPermissionChecked = true;
        }

        if (m_appHasDrmPermission) {
            // When the application wants to play back DRM secured media, we can't use
            // the QVideoRendererControl, because we won't have access to the pixel data
            // in this case.
            return 0;
        }

        if (!m_videoRendererControl) {
            m_videoRendererControl = new MmRendererPlayerVideoRendererControl();
            updateControls();
        }
        return m_videoRendererControl;
    } else if (qstrcmp(name, QVideoWindowControl_iid) == 0) {
        if (!m_videoWindowControl) {
            m_videoWindowControl = new MmRendererVideoWindowControl();
            updateControls();
        }
        return m_videoWindowControl;
    }
    return 0;
}

void MmRendererMediaPlayerService::releaseControl(QMediaControl *control)
{
    if (control == m_videoRendererControl)
        m_videoRendererControl = 0;
    if (control == m_videoWindowControl)
        m_videoWindowControl = 0;
    if (control == m_mediaPlayerControl)
        m_mediaPlayerControl = 0;
    if (control == m_metaDataReaderControl)
        m_metaDataReaderControl = 0;
    if (control == m_audioRoleControl)
        m_audioRoleControl = 0;
    if (control == m_customAudioRoleControl)
        m_customAudioRoleControl = 0;
    delete control;
}

void MmRendererMediaPlayerService::updateControls()
{
    if (m_videoRendererControl && m_mediaPlayerControl)
        m_mediaPlayerControl->setVideoRendererControl(m_videoRendererControl);

    if (m_videoWindowControl && m_mediaPlayerControl)
        m_mediaPlayerControl->setVideoWindowControl(m_videoWindowControl);

    if (m_metaDataReaderControl && m_mediaPlayerControl)
        m_mediaPlayerControl->setMetaDataReaderControl(m_metaDataReaderControl);

    if (m_audioRoleControl && m_mediaPlayerControl)
        m_mediaPlayerControl->setAudioRoleControl(m_audioRoleControl);

    if (m_customAudioRoleControl && m_mediaPlayerControl)
        m_mediaPlayerControl->setCustomAudioRoleControl(m_customAudioRoleControl);
}

QT_END_NAMESPACE
