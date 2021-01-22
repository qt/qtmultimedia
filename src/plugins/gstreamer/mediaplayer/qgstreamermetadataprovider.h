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

#ifndef QGSTREAMERMETADATAPROVIDER_H
#define QGSTREAMERMETADATAPROVIDER_H

#include <qmetadatareadercontrol.h>

QT_BEGIN_NAMESPACE

class QGstreamerPlayerSession;

class QGstreamerMetaDataProvider : public QMetaDataReaderControl
{
    Q_OBJECT
public:
    QGstreamerMetaDataProvider( QGstreamerPlayerSession *session, QObject *parent );
    virtual ~QGstreamerMetaDataProvider();

    bool isMetaDataAvailable() const override;
    bool isWritable() const;

    QVariant metaData(const QString &key) const override;
    QStringList availableMetaData() const override;

private slots:
    void updateTags();

private:
    QGstreamerPlayerSession *m_session = nullptr;
    QVariantMap m_tags;
};

QT_END_NAMESPACE

#endif // QGSTREAMERMETADATAPROVIDER_H
