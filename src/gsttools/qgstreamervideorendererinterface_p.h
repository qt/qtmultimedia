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

#ifndef QGSTREAMERVIDEOOUTPUTCONTROL_H
#define QGSTREAMERVIDEOOUTPUTCONTROL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <gst/gst.h>

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QGstreamerVideoRendererInterface
{
public:
    virtual ~QGstreamerVideoRendererInterface();
    virtual GstElement *videoSink() = 0;
    virtual void setVideoSink(GstElement *) {};

    //stopRenderer() is called when the renderer element is stopped.
    //it can be reimplemented when video renderer can't detect
    //changes to NULL state but has to free video resources.
    virtual void stopRenderer() {}

    //the video output is configured, usually after the first paint event
    //(winId is known,
    virtual bool isReady() const { return true; }

    //signals:
    //void sinkChanged();
    //void readyChanged(bool);
};

#define QGstreamerVideoRendererInterface_iid "org.qt-project.qt.gstreamervideorenderer/5.0"
Q_DECLARE_INTERFACE(QGstreamerVideoRendererInterface, QGstreamerVideoRendererInterface_iid)
QT_END_NAMESPACE

#endif
