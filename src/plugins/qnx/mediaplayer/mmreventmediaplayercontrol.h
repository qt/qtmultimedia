/****************************************************************************
**
** Copyright (C) 2017 QNX Software Systems. All rights reserved.
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
#ifndef MMREVENTMEDIAPLAYERCONTROL_H
#define MMREVENTMEDIAPLAYERCONTROL_H

#include "mmrenderermediaplayercontrol.h"

#include <mm/renderer/events.h>

QT_BEGIN_NAMESPACE

class MmrEventThread;

class MmrEventMediaPlayerControl final : public MmRendererMediaPlayerControl
{
    Q_OBJECT
public:
    explicit MmrEventMediaPlayerControl(QObject *parent = 0);
    ~MmrEventMediaPlayerControl() override;

    void startMonitoring() override;
    void stopMonitoring() override;
    void resetMonitoring() override;

    bool nativeEventFilter(const QByteArray &eventType,
                           void *message,
                           long *result) override;

private Q_SLOTS:
    void readEvents();

private:
    MmrEventThread *m_eventThread;

    // status properties.
    QByteArray m_bufferStatus;
    int m_bufferLevel;
    int m_bufferCapacity;
    qint64 m_position;
    bool m_suspended;
    QByteArray m_suspendedReason;

    // state properties.
    mmr_state_t m_state;
    int m_speed;
};

QT_END_NAMESPACE

#endif
