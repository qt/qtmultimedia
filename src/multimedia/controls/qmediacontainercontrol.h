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


#ifndef QMEDIACONTAINERCONTROL_H
#define QMEDIACONTAINERCONTROL_H

#include <QtMultimedia/qmediacontrol.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QMediaContainerControl : public QMediaControl
{
    Q_OBJECT

public:
    virtual ~QMediaContainerControl();

    virtual QStringList supportedContainers() const = 0;
    virtual QString containerFormat() const = 0;
    virtual void setContainerFormat(const QString &format) = 0;

    virtual QString containerDescription(const QString &formatMimeType) const = 0;

protected:
    explicit QMediaContainerControl(QObject *parent = nullptr);
};

#define QMediaContainerControl_iid "org.qt-project.qt.mediacontainercontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QMediaContainerControl, QMediaContainerControl_iid)

QT_END_NAMESPACE


#endif // QMEDIACONTAINERCONTROL_H
