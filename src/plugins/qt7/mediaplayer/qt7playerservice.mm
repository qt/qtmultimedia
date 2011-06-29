/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>
#include <QtGui/qwidget.h>

#include "qt7backend.h"
#include "qt7playerservice.h"
#include "qt7playercontrol.h"
#include "qt7playersession.h"
#include "qt7videooutput.h"
#include "qt7movieviewoutput.h"
#include "qt7movieviewrenderer.h"
#include "qt7movierenderer.h"
#include "qt7movievideowidget.h"
#include "qt7playermetadata.h"

#include <qmediaplaylistnavigator.h>
#include <qmediaplaylist.h>

QT_USE_NAMESPACE

QT7PlayerService::QT7PlayerService(QObject *parent):
    QMediaService(parent),
    m_videoOutput(0)
{
    m_session = new QT7PlayerSession(this);

    m_control = new QT7PlayerControl(this);
    m_control->setSession(m_session);

    m_playerMetaDataControl = new QT7PlayerMetaDataControl(m_session, this);
    connect(m_control, SIGNAL(mediaChanged(QMediaContent)), m_playerMetaDataControl, SLOT(updateTags()));
}

QT7PlayerService::~QT7PlayerService()
{
}

QMediaControl *QT7PlayerService::requestControl(const char *name)
{
    if (qstrcmp(name, QMediaPlayerControl_iid) == 0)
        return m_control;

    if (qstrcmp(name, QMetaDataReaderControl_iid) == 0)
        return m_playerMetaDataControl;

#ifndef QT_NO_OPENGL
    if (!m_videoOutput) {
        if (qstrcmp(name, QVideoWindowControl_iid) == 0) {
#if defined(QT_MAC_USE_COCOA)
            m_videoOutput = new QT7MovieViewOutput(this);
#endif
        }

        if (qstrcmp(name, QVideoRendererControl_iid) == 0) {
#ifdef QUICKTIME_C_API_AVAILABLE
            m_videoOutput = new QT7MovieRenderer(this);
#else
            m_videoOutput = new QT7MovieViewRenderer(this);
#endif
        }

        if (qstrcmp(name, QVideoWidgetControl_iid) == 0) {
#ifdef QUICKTIME_C_API_AVAILABLE
            m_videoOutput = new QT7MovieVideoWidget(this);
#endif
        }

        if (m_videoOutput) {
            QT7VideoOutput *videoOutput = qobject_cast<QT7VideoOutput*>(m_videoOutput);
            m_session->setVideoOutput(videoOutput);
            return m_videoOutput;
        }
    }
#endif // !defined(QT_NO_OPENGL)

    return 0;
}

void QT7PlayerService::releaseControl(QMediaControl *control)
{
    if (m_videoOutput == control) {
        m_videoOutput = 0;
        m_session->setVideoOutput(0);
        delete control;
    }
}

#include "moc_qt7playerservice.cpp"
