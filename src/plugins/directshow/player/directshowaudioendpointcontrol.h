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

#ifndef DIRECTSHOWAUDIOENDPOINTCONTROL_H
#define DIRECTSHOWAUDIOENDPOINTCONTROL_H

#include "qaudiooutputselectorcontrol.h"

#include <dshow.h>

QT_BEGIN_NAMESPACE

class DirectShowPlayerService;

class DirectShowAudioEndpointControl : public QAudioOutputSelectorControl
{
    Q_OBJECT
public:
    DirectShowAudioEndpointControl(DirectShowPlayerService *service, QObject *parent = nullptr);
    ~DirectShowAudioEndpointControl() override;

    QList<QString> availableOutputs() const override;

    QString outputDescription(const QString &name) const override;

    QString defaultOutput() const override;
    QString activeOutput() const override;

    void setActiveOutput(const QString& name) override;

private:
    void updateEndpoints();

    DirectShowPlayerService *m_service;
    IBindCtx *m_bindContext = nullptr;
    ICreateDevEnum *m_deviceEnumerator = nullptr;

    QMap<QString, IMoniker *> m_devices;
    QString m_defaultEndpoint;
    QString m_activeEndpoint;
};

QT_END_NAMESPACE

#endif

