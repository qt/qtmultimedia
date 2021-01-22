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

#ifndef QCAMERAVIEWFINDER_H
#define QCAMERAVIEWFINDER_H

#include <QtCore/qstringlist.h>
#include <QtCore/qpair.h>
#include <QtCore/qsize.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qmediaobject.h>
#include <QtMultimedia/qmediaservice.h>
#include <QtMultimediaWidgets/qvideowidget.h>

QT_BEGIN_NAMESPACE


class QCamera;

class QCameraViewfinderPrivate;
class Q_MULTIMEDIAWIDGETS_EXPORT QCameraViewfinder : public QVideoWidget
{
    Q_OBJECT
public:
    explicit QCameraViewfinder(QWidget *parent = nullptr);
    ~QCameraViewfinder();

    QMediaObject *mediaObject() const override;

protected:
    bool setMediaObject(QMediaObject *object) override;

private:
    Q_DISABLE_COPY(QCameraViewfinder)
    Q_DECLARE_PRIVATE(QCameraViewfinder)
};

QT_END_NAMESPACE


#endif  // QCAMERA_H
