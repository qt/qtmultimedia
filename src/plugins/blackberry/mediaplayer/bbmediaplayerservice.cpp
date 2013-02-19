/****************************************************************************
**
** Copyright (C) 2012 Research In Motion
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "bbmediaplayerservice.h"

#include "bbmediaplayercontrol.h"
#include "bbmetadatareadercontrol.h"
#include "bbplayervideorenderercontrol.h"
#include "bbutil.h"
#include "bbvideowindowcontrol.h"

QT_BEGIN_NAMESPACE

BbMediaPlayerService::BbMediaPlayerService(QObject *parent)
    : QMediaService(parent),
      m_videoRendererControl(0),
      m_videoWindowControl(0),
      m_mediaPlayerControl(0),
      m_metaDataReaderControl(0),
      m_appHasDrmPermission(false),
      m_appHasDrmPermissionChecked(false)
{
}

BbMediaPlayerService::~BbMediaPlayerService()
{
    // Someone should have called releaseControl(), but better be safe
    delete m_videoRendererControl;
    delete m_videoWindowControl;
    delete m_mediaPlayerControl;
    delete m_metaDataReaderControl;
}

QMediaControl *BbMediaPlayerService::requestControl(const char *name)
{
    if (qstrcmp(name, QMediaPlayerControl_iid) == 0) {
        if (!m_mediaPlayerControl) {
            m_mediaPlayerControl = new BbMediaPlayerControl();
            updateControls();
        }
        return m_mediaPlayerControl;
    }
    else if (qstrcmp(name, QMetaDataReaderControl_iid) == 0) {
        if (!m_metaDataReaderControl) {
            m_metaDataReaderControl = new BbMetaDataReaderControl();
            updateControls();
        }
        return m_metaDataReaderControl;
    }
    else if (qstrcmp(name, QVideoRendererControl_iid) == 0) {
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
            m_videoRendererControl = new BbPlayerVideoRendererControl();
            updateControls();
        }
        return m_videoRendererControl;
    }
    else if (qstrcmp(name, QVideoWindowControl_iid) == 0) {
        if (!m_videoWindowControl) {
            m_videoWindowControl = new BbVideoWindowControl();
            updateControls();
        }
        return m_videoWindowControl;
    }
    return 0;
}

void BbMediaPlayerService::releaseControl(QMediaControl *control)
{
    if (control == m_videoRendererControl)
        m_videoRendererControl = 0;
    if (control == m_videoWindowControl)
        m_videoWindowControl = 0;
    if (control == m_mediaPlayerControl)
        m_mediaPlayerControl = 0;
    if (control == m_metaDataReaderControl)
        m_metaDataReaderControl = 0;
    delete control;
}

void BbMediaPlayerService::updateControls()
{
    if (m_videoRendererControl && m_mediaPlayerControl)
        m_mediaPlayerControl->setVideoRendererControl(m_videoRendererControl);

    if (m_videoWindowControl && m_mediaPlayerControl)
        m_mediaPlayerControl->setVideoWindowControl(m_videoWindowControl);

    if (m_metaDataReaderControl && m_mediaPlayerControl)
        m_mediaPlayerControl->setMetaDataReaderControl(m_metaDataReaderControl);
}

QT_END_NAMESPACE
