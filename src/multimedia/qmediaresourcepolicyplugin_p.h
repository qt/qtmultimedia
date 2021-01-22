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

#ifndef QRESOURCEPOLICYPLUGIN_P_H
#define QRESOURCEPOLICYPLUGIN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QObject>
#include <qtmultimediaglobal.h>

QT_BEGIN_NAMESPACE

struct Q_MULTIMEDIA_EXPORT QMediaResourceSetFactoryInterface
{
    virtual QObject* create(const QString& interfaceId) = 0;
    virtual void destroy(QObject *resourceSet) = 0;
};

#define QMediaResourceSetFactoryInterface_iid \
    "org.qt-project.qt.mediaresourcesetfactory/5.0"
Q_DECLARE_INTERFACE(QMediaResourceSetFactoryInterface, QMediaResourceSetFactoryInterface_iid)

class Q_MULTIMEDIA_EXPORT QMediaResourcePolicyPlugin : public QObject, public QMediaResourceSetFactoryInterface
{
    Q_OBJECT
    Q_INTERFACES(QMediaResourceSetFactoryInterface)

public:
    QMediaResourcePolicyPlugin(QObject *parent = nullptr);
    ~QMediaResourcePolicyPlugin();
};

QT_END_NAMESPACE

#endif // QRESOURCEPOLICYPLUGIN_P_H
