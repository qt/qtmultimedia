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



#ifndef QCAMERAFEEDBACKCONTROL_H
#define QCAMERAFEEDBACKCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qmediaobject.h>

#include <QtMultimedia/qcamera.h>
#include <QtMultimedia/qmediaenumdebug.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QCameraFeedbackControl : public QMediaControl
{
    Q_OBJECT

public:
    enum EventType {
        ViewfinderStarted = 1,
        ViewfinderStopped,
        ImageCaptured,
        ImageSaved,
        ImageError,
        RecordingStarted,
        RecordingInProgress,
        RecordingStopped,
        AutoFocusInProgress,
        AutoFocusLocked,
        AutoFocusFailed
    };

    ~QCameraFeedbackControl();

    virtual bool isEventFeedbackLocked(EventType) const = 0;

    virtual bool isEventFeedbackEnabled(EventType) const = 0;

    virtual bool setEventFeedbackEnabled(EventType, bool) = 0;
    virtual void resetEventFeedback(EventType) = 0;

    virtual bool setEventFeedbackSound(EventType, const QString &filePath) = 0;

protected:
    explicit QCameraFeedbackControl(QObject *parent = nullptr);
};

#define QCameraFeedbackControl_iid "org.qt-project.qt.camerafeedbackcontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QCameraFeedbackControl, QCameraFeedbackControl_iid)

QT_END_NAMESPACE


#endif // QCAMERAFEEDBACKCONTROL_H
