/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>

#include "qt7backend.h"
#include "qt7playerservice.h"
#include "qt7playercontrol.h"
#include "qt7playersession.h"
#include "qt7videooutput.h"
#include "qt7movieviewoutput.h"
#include "qt7movieviewrenderer.h"
#include "qt7movierenderer.h"
#ifndef QT_NO_WIDGETS
#include "qt7movievideowidget.h"
#endif
#include "qt7playermetadata.h"

#include <private/qmediaplaylistnavigator_p.h>
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
            m_videoOutput = new QT7MovieViewOutput(this);
        }

        if (qstrcmp(name, QVideoRendererControl_iid) == 0) {
#ifndef QT_NO_WIDGETS
            m_videoOutput = new QT7MovieViewRenderer(this);
#endif
        }

#ifndef QT_NO_WIDGETS
        if (qstrcmp(name, QVideoWidgetControl_iid) == 0) {
#ifdef QUICKTIME_C_API_AVAILABLE
            m_videoOutput = new QT7MovieVideoWidget(this);
#endif
        }
#endif

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
