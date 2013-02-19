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

#ifndef DSCAMERACONTROL_H
#define DSCAMERACONTROL_H

#include <QtCore/qobject.h>
#include <qcameracontrol.h>

QT_BEGIN_NAMESPACE

class DSCameraService;
class DSCameraSession;


class DSCameraControl : public QCameraControl
{
    Q_OBJECT
public:
    DSCameraControl(QObject *parent = 0);
    ~DSCameraControl();

    void start();
    void stop();
    QCamera::State state() const;

    QCamera::CaptureModes captureMode() const { return m_captureMode; }
    void setCaptureMode(QCamera::CaptureModes mode)
    {
        if (m_captureMode != mode) {
            m_captureMode = mode;
            emit captureModeChanged(mode);
        }
    }

    void setState(QCamera::State state);

    QCamera::Status status() const { return QCamera::UnavailableStatus; }
    bool isCaptureModeSupported(QCamera::CaptureModes mode) const;
    bool canChangeProperty(PropertyChangeType /* changeType */, QCamera::Status /* status */) const {return false; }

private:
    DSCameraSession *m_session;
    DSCameraService *m_service;
    QCamera::CaptureModes m_captureMode;
};

QT_END_NAMESPACE

#endif


