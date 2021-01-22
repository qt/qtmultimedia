/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Mobility Components.
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

#ifndef MFAUDIOENDPOINTCONTROL_H
#define MFAUDIOENDPOINTCONTROL_H

#include <mfapi.h>
#include <mfidl.h>

#include "qaudiooutputselectorcontrol.h"

class MFPlayerService;

QT_USE_NAMESPACE

class MFAudioEndpointControl : public QAudioOutputSelectorControl
{
    Q_OBJECT
public:
    MFAudioEndpointControl(QObject *parent = 0);
    ~MFAudioEndpointControl();

    QList<QString> availableOutputs() const;

    QString outputDescription(const QString &name) const;

    QString defaultOutput() const;
    QString activeOutput() const;

    void setActiveOutput(const QString& name);

    IMFActivate* createActivate();

private:
    void clear();
    void updateEndpoints();

    QString m_defaultEndpoint;
    QString m_activeEndpoint;
    QMap<QString, LPWSTR> m_devices;
    IMFActivate *m_currentActivate;

};

#endif

