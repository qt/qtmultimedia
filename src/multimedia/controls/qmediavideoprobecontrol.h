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



#ifndef QMEDIAVIDEOPROBECONTROL_H
#define QMEDIAVIDEOPROBECONTROL_H

#include <QtMultimedia/qmediacontrol.h>

QT_BEGIN_NAMESPACE

class QVideoFrame;
class Q_MULTIMEDIA_EXPORT QMediaVideoProbeControl : public QMediaControl
{
    Q_OBJECT
public:
    virtual ~QMediaVideoProbeControl();

Q_SIGNALS:
    void videoFrameProbed(const QVideoFrame &frame);
    void flush();

protected:
    explicit QMediaVideoProbeControl(QObject *parent = nullptr);
};

#define QMediaVideoProbeControl_iid "org.qt-project.qt.mediavideoprobecontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QMediaVideoProbeControl, QMediaVideoProbeControl_iid)

QT_END_NAMESPACE


#endif // QMEDIAVIDEOPROBECONTROL_H
