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

#include <QtCore/QDebug>

#include "dsimagecapturecontrol.h"

QT_BEGIN_NAMESPACE

DSImageCaptureControl::DSImageCaptureControl(DSCameraSession *session)
    :QCameraImageCaptureControl(session), m_session(session), m_ready(false)
{
    connect(m_session, SIGNAL(stateChanged(QCamera::State)), SLOT(updateState()));
    connect(m_session, SIGNAL(imageCaptured(const int, QImage)),
        this, SIGNAL(imageCaptured(const int, QImage)));
    connect(m_session, SIGNAL(imageSaved(const int, const QString &)),
            this, SIGNAL(imageSaved(const int, const QString &)));
    connect(m_session, SIGNAL(readyForCaptureChanged(bool)),
            this, SIGNAL(readyForCaptureChanged(bool)));
}

DSImageCaptureControl::~DSImageCaptureControl()
{
}

bool DSImageCaptureControl::isReadyForCapture() const
{
    return m_ready;
}

int DSImageCaptureControl::capture(const QString &fileName)
{
   return m_session->captureImage(fileName);
}

void DSImageCaptureControl::updateState()
{
    bool ready = (m_session->state() == QCamera::ActiveState) &&
                 !m_session->pictureInProgress();
    if(m_ready != ready)
        emit readyForCaptureChanged(m_ready = ready);
}

QT_END_NAMESPACE

