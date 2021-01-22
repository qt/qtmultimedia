/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QDECLARATIVECAMERAVIEWFINDER_P_H
#define QDECLARATIVECAMERAVIEWFINDER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qcamera.h>
#include <qmediarecorder.h>
#include <qmediaencodersettings.h>

QT_BEGIN_NAMESPACE

class QDeclarativeCamera;
class QCameraViewfinderSettingsControl;

class QDeclarativeCameraViewfinder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QSize resolution READ resolution WRITE setResolution NOTIFY resolutionChanged)
    Q_PROPERTY(qreal minimumFrameRate READ minimumFrameRate WRITE setMinimumFrameRate NOTIFY minimumFrameRateChanged)
    Q_PROPERTY(qreal maximumFrameRate READ maximumFrameRate WRITE setMaximumFrameRate NOTIFY maximumFrameRateChanged)
public:
    QDeclarativeCameraViewfinder(QCamera *camera, QObject *parent = 0);
    ~QDeclarativeCameraViewfinder();

    QSize resolution() const;
    void setResolution(const QSize &resolution);

    qreal minimumFrameRate() const;
    void setMinimumFrameRate(qreal frameRate);

    qreal maximumFrameRate() const;
    void setMaximumFrameRate(qreal frameRate);

Q_SIGNALS:
    void resolutionChanged();
    void minimumFrameRateChanged();
    void maximumFrameRateChanged();

private Q_SLOTS:
    void _q_cameraStatusChanged(QCamera::Status status);

private:
    QCamera *m_camera;
    QCameraViewfinderSettings m_settings;
};

QT_END_NAMESPACE

#endif
