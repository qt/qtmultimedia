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

#ifndef AVFCAMERAMETADATACONTROL_H
#define AVFCAMERAMETADATACONTROL_H

#include <qmetadatawritercontrol.h>

QT_BEGIN_NAMESPACE

class AVFCameraService;

class AVFCameraMetaDataControl : public QMetaDataWriterControl
{
    Q_OBJECT
public:
    AVFCameraMetaDataControl(AVFCameraService *service, QObject *parent = nullptr);
    virtual ~AVFCameraMetaDataControl();

    bool isMetaDataAvailable() const override;
    bool isWritable() const override;

    QVariant metaData(const QString &key) const override;
    void setMetaData(const QString &key, const QVariant &value) override;
    QStringList availableMetaData() const override;

private:
    QMap<QString, QVariant> m_tags;
};

QT_END_NAMESPACE

#endif
