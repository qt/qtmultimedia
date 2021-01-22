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

#ifndef MFAUDIOPROBECONTROL_H
#define MFAUDIOPROBECONTROL_H

#include <qmediaaudioprobecontrol.h>
#include <QtCore/qmutex.h>
#include <qaudiobuffer.h>

QT_USE_NAMESPACE

class MFAudioProbeControl : public QMediaAudioProbeControl
{
    Q_OBJECT
public:
    explicit MFAudioProbeControl(QObject *parent);
    virtual ~MFAudioProbeControl();

    void bufferProbed(const char *data, quint32 size, const QAudioFormat& format, qint64 startTime);

private slots:
    void bufferProbed();

private:
    QAudioBuffer m_pendingBuffer;
    QMutex m_bufferMutex;
};

#endif
