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

#ifndef SIMULATORCAMERACAPTURESESSION_H
#define SIMULATORCAMERACAPTURESESSION_H

#include <qmediarecordercontrol.h>

#include <QtCore/qurl.h>
#include <QtCore/qdir.h>

#include "qcamera.h"

class SimulatorCameraSettings;

class SimulatorCameraSession : public QObject
{
    Q_OBJECT
public:
    SimulatorCameraSession(QObject *parent);
    ~SimulatorCameraSession();

    QCamera::CaptureMode captureMode();
    void setCaptureMode(QCamera::CaptureMode mode);

    QDir defaultDir(QCamera::CaptureMode mode) const;
    QString generateFileName(const QString &prefix, const QDir &dir, const QString &ext) const;

    void setImage(const QImage *image);
    QObject *viewfinder() const;
    void setViewfinder(QObject *viewfinder);

    int captureImage(const QString &fileName);

    SimulatorCameraSettings *settings() const;

signals:
    void stateChanged(QCamera::State state);
    void captureError(int id, int error, const QString &errorString);
    void error(int error, const QString &errorString);
    void imageExposed(int requestId);
    void imageCaptured(int requestId, const QImage &img);
    void imageSaved(int requestId, const QString &fileName);
    void viewfinderChanged();

private:
    QCamera::CaptureMode mCaptureMode;

    QObject *mViewfinder;
    const QImage *mImage;

    SimulatorCameraSettings *mSettings;

public:
    int mRequestId;
};

#endif // SIMULATORCAMERACAPTURESESSION_H
