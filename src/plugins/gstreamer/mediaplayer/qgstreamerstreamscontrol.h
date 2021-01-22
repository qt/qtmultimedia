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

#ifndef QGSTREAMERSTREAMSCONTROL_H
#define QGSTREAMERSTREAMSCONTROL_H

#include <qmediastreamscontrol.h>

QT_BEGIN_NAMESPACE

class QGstreamerPlayerSession;

class QGstreamerStreamsControl : public QMediaStreamsControl
{
    Q_OBJECT
public:
    QGstreamerStreamsControl(QGstreamerPlayerSession *session, QObject *parent);
    virtual ~QGstreamerStreamsControl();

    int streamCount() override;
    StreamType streamType(int streamNumber) override;

    QVariant metaData(int streamNumber, const QString &key) override;

    bool isActive(int streamNumber) override;
    void setActive(int streamNumber, bool state) override;

private:
    QGstreamerPlayerSession *m_session = nullptr;
};

QT_END_NAMESPACE

#endif // QGSTREAMERSTREAMSCONTROL_H

