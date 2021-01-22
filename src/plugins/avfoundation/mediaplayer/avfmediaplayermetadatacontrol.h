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

#ifndef AVFMEDIAPLAYERMETADATACONTROL_H
#define AVFMEDIAPLAYERMETADATACONTROL_H

#include <QtMultimedia/QMetaDataReaderControl>

QT_BEGIN_NAMESPACE

class AVFMediaPlayerSession;

class AVFMediaPlayerMetaDataControl : public QMetaDataReaderControl
{
    Q_OBJECT
public:
    explicit AVFMediaPlayerMetaDataControl(AVFMediaPlayerSession *session, QObject *parent = nullptr);
    virtual ~AVFMediaPlayerMetaDataControl();

    bool isMetaDataAvailable() const override;
    bool isWritable() const;

    QVariant metaData(const QString &key) const override;
    QStringList availableMetaData() const override;

private Q_SLOTS:
    void updateTags();

private:
    AVFMediaPlayerSession *m_session;
    QVariantMap m_tags;
    void *m_asset;

};

QT_END_NAMESPACE

#endif // AVFMEDIAPLAYERMETADATACONTROL_H
