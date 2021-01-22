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

#ifndef QCAMERAIMAGECAPTURECONTROL_H
#define QCAMERAIMAGECAPTURECONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qcameraimagecapture.h>

QT_BEGIN_NAMESPACE

class QImage;
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QCameraImageCaptureControl : public QMediaControl
{
    Q_OBJECT

public:
    ~QCameraImageCaptureControl();

    virtual bool isReadyForCapture() const = 0;

    virtual QCameraImageCapture::DriveMode driveMode() const = 0;
    virtual void setDriveMode(QCameraImageCapture::DriveMode mode) = 0;

    virtual int capture(const QString &fileName) = 0;
    virtual void cancelCapture() = 0;

Q_SIGNALS:
    void readyForCaptureChanged(bool ready);

    void imageExposed(int requestId);
    void imageCaptured(int requestId, const QImage &preview);
    void imageMetadataAvailable(int id, const QString &key, const QVariant &value);
    void imageAvailable(int requestId, const QVideoFrame &buffer);
    void imageSaved(int requestId, const QString &fileName);

    void error(int id, int error, const QString &errorString);

protected:
    explicit QCameraImageCaptureControl(QObject *parent = nullptr);
};

#define QCameraImageCaptureControl_iid "org.qt-project.qt.cameraimagecapturecontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QCameraImageCaptureControl, QCameraImageCaptureControl_iid)

QT_END_NAMESPACE


#endif  // QCAMERAIMAGECAPTURECONTROL_H

