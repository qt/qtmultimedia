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

#ifndef QANDROIDMETADATAREADERCONTROL_H
#define QANDROIDMETADATAREADERCONTROL_H

#include <QMetaDataReaderControl>
#include <qmediacontent.h>
#include <QMutex>

QT_BEGIN_NAMESPACE

class AndroidMediaMetadataRetriever;

class QAndroidMetaDataReaderControl : public QMetaDataReaderControl
{
    Q_OBJECT
public:
    explicit QAndroidMetaDataReaderControl(QObject *parent = 0);
    ~QAndroidMetaDataReaderControl() override;

    bool isMetaDataAvailable() const override;

    QVariant metaData(const QString &key) const override;
    QStringList availableMetaData() const override;

public Q_SLOTS:
    void onMediaChanged(const QMediaContent &media);
    void onUpdateMetaData();

private:
    void updateData(const QVariantMap &metadata, const QUrl &url);
    static void extractMetadata(QAndroidMetaDataReaderControl *caller, const QUrl &url);

    mutable QMutex m_mtx;
    QMediaContent m_mediaContent;
    bool m_available;
    QVariantMap m_metadata;
};

QT_END_NAMESPACE

#endif // QANDROIDMETADATAREADERCONTROL_H
