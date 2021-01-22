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


#ifndef QMEDIASTREAMSCONTROL_H
#define QMEDIASTREAMSCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qmultimedia.h>
#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qmediaenumdebug.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QMediaStreamsControl : public QMediaControl
{
    Q_OBJECT
    Q_ENUMS(SteamType)
public:
    enum StreamType { UnknownStream, VideoStream, AudioStream, SubPictureStream, DataStream };

    virtual ~QMediaStreamsControl();

    virtual int streamCount() = 0;
    virtual StreamType streamType(int streamNumber) = 0;

    virtual QVariant metaData(int streamNumber, const QString &key) = 0;

    virtual bool isActive(int streamNumber) = 0;
    virtual void setActive(int streamNumber, bool state) = 0;

Q_SIGNALS:
    void streamsChanged();
    void activeStreamsChanged();

protected:
    explicit QMediaStreamsControl(QObject *parent = nullptr);
};

#define QMediaStreamsControl_iid "org.qt-project.qt.mediastreamscontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QMediaStreamsControl, QMediaStreamsControl_iid)

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QMediaStreamsControl::StreamType)

Q_MEDIA_ENUM_DEBUG(QMediaStreamsControl, StreamType)

#endif // QMEDIASTREAMSCONTROL_H

