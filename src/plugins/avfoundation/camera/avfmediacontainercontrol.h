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

#ifndef AVFMEDIACONTAINERCONTROL_H
#define AVFMEDIACONTAINERCONTROL_H

#include <qmediacontainercontrol.h>

@class NSString;

QT_BEGIN_NAMESPACE

class AVFCameraService;

class AVFMediaContainerControl : public QMediaContainerControl
{
public:
    explicit AVFMediaContainerControl(AVFCameraService *service);

    QStringList supportedContainers() const override;
    QString containerFormat() const override;
    void setContainerFormat(const QString &format) override;
    QString containerDescription(const QString &formatMimeType) const override;

    NSString *fileType() const;

private:
    QString m_format;
};

QT_END_NAMESPACE

#endif // AVFMEDIACONTAINERCONTROL_H
