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

#ifndef QMEDIAAUDIOPROBECONTROL_H
#define QMEDIAAUDIOPROBECONTROL_H

#include <QtMultimedia/qmediacontrol.h>

QT_BEGIN_NAMESPACE

class QAudioBuffer;
class Q_MULTIMEDIA_EXPORT QMediaAudioProbeControl : public QMediaControl
{
    Q_OBJECT
public:
    virtual ~QMediaAudioProbeControl();

Q_SIGNALS:
    void audioBufferProbed(const QAudioBuffer &buffer);
    void flush();

protected:
    explicit QMediaAudioProbeControl(QObject *parent = nullptr);
};

#define QMediaAudioProbeControl_iid "org.qt-project.qt.mediaaudioprobecontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QMediaAudioProbeControl, QMediaAudioProbeControl_iid)

QT_END_NAMESPACE


#endif // QMEDIAAUDIOPROBECONTROL_H
