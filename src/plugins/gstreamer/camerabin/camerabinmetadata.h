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

#ifndef CAMERABINCAPTUREMETADATACONTROL_H
#define CAMERABINCAPTUREMETADATACONTROL_H

#include <qmetadatawritercontrol.h>

QT_BEGIN_NAMESPACE

class CameraBinMetaData : public QMetaDataWriterControl
{
    Q_OBJECT
public:
    CameraBinMetaData(QObject *parent);
    virtual ~CameraBinMetaData() {}


    bool isMetaDataAvailable() const override { return true; }
    bool isWritable() const override { return true; }

    QVariant metaData(const QString &key) const override;
    void setMetaData(const QString &key, const QVariant &value) override;
    QStringList availableMetaData() const override;

Q_SIGNALS:
    void metaDataChanged(const QMap<QByteArray, QVariant>&);

private:
    QMap<QByteArray, QVariant> m_values;
};

QT_END_NAMESPACE

#endif // CAMERABINCAPTUREMETADATACONTROL_H
