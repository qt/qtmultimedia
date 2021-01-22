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

#ifndef DIRECTSHOWMETADATACONTROL_H
#define DIRECTSHOWMETADATACONTROL_H

#include <dshow.h>

#include <qmetadatareadercontrol.h>

#include "directshowglobal.h"

#include <QtCore/qcoreevent.h>

QT_BEGIN_NAMESPACE

class DirectShowMetaDataControl : public QMetaDataReaderControl
{
    Q_OBJECT
public:
    DirectShowMetaDataControl(QObject *parent = nullptr);
    ~DirectShowMetaDataControl() override;

    bool isMetaDataAvailable() const override;

    QVariant metaData(const QString &key) const override;
    QStringList availableMetaData() const override;

    void setMetadata(const QVariantMap &metadata);

    static void updateMetadata(const QString &fileSrc, QVariantMap &metadata);
    static void updateMetadata(IFilterGraph2 *graph, IBaseFilter *source, QVariantMap &metadata);

private:
    void setMetadataAvailable(bool available);

    enum Event
    {
        MetaDataChanged = QEvent::User
    };

    QVariantMap m_metadata;
    bool m_available = false;
};

QT_END_NAMESPACE

#endif
