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

#ifndef QABSTRACTMEDIAOBJECT_H
#define QABSTRACTMEDIAOBJECT_H

#include <QtCore/qobject.h>
#include <QtCore/qstringlist.h>

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qmultimedia.h>

QT_BEGIN_NAMESPACE


class QMediaService;
class QMediaBindableInterface;

class QMediaObjectPrivate;
class Q_MULTIMEDIA_EXPORT QMediaObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int notifyInterval READ notifyInterval WRITE setNotifyInterval NOTIFY notifyIntervalChanged)
public:
    ~QMediaObject();

    virtual bool isAvailable() const;
    virtual QMultimedia::AvailabilityStatus availability() const;

    virtual QMediaService* service() const;

    int notifyInterval() const;
    void setNotifyInterval(int milliSeconds);

    virtual bool bind(QObject *);
    virtual void unbind(QObject *);

    bool isMetaDataAvailable() const;

    QVariant metaData(const QString &key) const;
    QStringList availableMetaData() const;

Q_SIGNALS:
    void notifyIntervalChanged(int milliSeconds);

    void metaDataAvailableChanged(bool available);
    void metaDataChanged();
    void metaDataChanged(const QString &key, const QVariant &value);

    void availabilityChanged(bool available);
    void availabilityChanged(QMultimedia::AvailabilityStatus availability);

protected:
    QMediaObject(QObject *parent, QMediaService *service);
    QMediaObject(QMediaObjectPrivate &dd, QObject *parent, QMediaService *service);

    void addPropertyWatch(QByteArray const &name);
    void removePropertyWatch(QByteArray const &name);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QMediaObjectPrivate *d_ptr_deprecated;
#endif

private:
    void setupControls();

    Q_DECLARE_PRIVATE(QMediaObject)
    Q_PRIVATE_SLOT(d_func(), void _q_notify())
    Q_PRIVATE_SLOT(d_func(), void _q_availabilityChanged())
};


QT_END_NAMESPACE


#endif  // QABSTRACTMEDIAOBJECT_H
