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

#ifndef QGSTREAMERAUDIODECODERSERVICE_H
#define QGSTREAMERAUDIODECODERSERVICE_H

#include <QtCore/qobject.h>
#include <QtCore/qiodevice.h>

#include <qmediaservice.h>

QT_BEGIN_NAMESPACE
class QGstreamerAudioDecoderControl;
class QGstreamerAudioDecoderSession;

class QGstreamerAudioDecoderService : public QMediaService
{
    Q_OBJECT
public:
    QGstreamerAudioDecoderService(QObject *parent = 0);
    ~QGstreamerAudioDecoderService();

    QMediaControl *requestControl(const char *name) override;
    void releaseControl(QMediaControl *control) override;

private:
    QGstreamerAudioDecoderControl *m_control;
    QGstreamerAudioDecoderSession *m_session;
};

QT_END_NAMESPACE

#endif
