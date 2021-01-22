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

#ifndef QVIDEORENDERERCONTROL_H
#define QVIDEORENDERERCONTROL_H

#include <QtMultimedia/qmediacontrol.h>

QT_BEGIN_NAMESPACE

class QAbstractVideoSurface;
class Q_MULTIMEDIA_EXPORT QVideoRendererControl : public QMediaControl
{
    Q_OBJECT

public:
    ~QVideoRendererControl();

    virtual QAbstractVideoSurface *surface() const = 0;
    virtual void setSurface(QAbstractVideoSurface *surface) = 0;

protected:
    explicit QVideoRendererControl(QObject *parent = nullptr);
};

#define QVideoRendererControl_iid "org.qt-project.qt.videorenderercontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QVideoRendererControl, QVideoRendererControl_iid)

QT_END_NAMESPACE


#endif // QVIDEORENDERERCONTROL_H
