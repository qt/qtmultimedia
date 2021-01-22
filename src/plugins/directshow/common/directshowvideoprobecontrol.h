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

#ifndef DIRECTSHOWVIDEOPROBECONTROL_H
#define DIRECTSHOWVIDEOPROBECONTROL_H

#include <qmediavideoprobecontrol.h>

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

class DirectShowVideoProbeControl : public QMediaVideoProbeControl
{
    Q_OBJECT
public:
    explicit DirectShowVideoProbeControl(QObject *p = nullptr);
    ~DirectShowVideoProbeControl();

    bool ref() { return m_ref.ref(); }
    bool deref() { return m_ref.deref(); }
    void probeVideoFrame(const QVideoFrame &frame);
    void flushVideoFrame();
private:
    QAtomicInt m_ref;
    bool m_frameProbed = false;
};

QT_END_NAMESPACE

#endif // DIRECTSHOWVIDEOPROBECONTROL_H
