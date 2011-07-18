/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef TST_QMEDIAOBJECT_H
#define TST_QMEDIAOBJECT_H

#include <QtTest/QtTest>

#include <QtCore/qtimer.h>

#include <qmediaobject.h>
#include <qmediaservice.h>
#include <qmetadatareadercontrol.h>

//TESTED_COMPONENT=src/multimedia

QT_USE_NAMESPACE
class tst_QMediaObject : public QObject
{
    Q_OBJECT

private slots:
    void propertyWatch();
    void notifySignals_data();
    void notifySignals();
    void notifyInterval_data();
    void notifyInterval();

    void nullMetaDataControl();
    void isMetaDataAvailable();
    void metaDataChanged();
    void metaData_data();
    void metaData();
    void availability();
    void extendedMetaData_data() { metaData_data(); }
    void extendedMetaData();


private:
    void setupNotifyTests();
};

class QtTestMetaDataProvider : public QMetaDataReaderControl
{
    Q_OBJECT
public:
    QtTestMetaDataProvider(QObject *parent = 0)
        : QMetaDataReaderControl(parent)
        , m_available(false)
    {
    }

    bool isMetaDataAvailable() const { return m_available; }
    void setMetaDataAvailable(bool available) {
        if (m_available != available)
            emit metaDataAvailableChanged(m_available = available);
    }
    QList<QtMultimediaKit::MetaData> availableMetaData() const { return m_data.keys(); }


    QVariant metaData(QtMultimediaKit::MetaData key) const { return m_data.value(key); }

    QVariant extendedMetaData(const QString &key) const { return m_extendedData.value(key); }

    QStringList availableExtendedMetaData() const { return m_extendedData.keys(); }

    using QMetaDataReaderControl::metaDataChanged;

    void populateMetaData()
    {
        m_available = true;
    }

    bool m_available;
    QMap<QtMultimediaKit::MetaData, QVariant> m_data;
    QMap<QString, QVariant> m_extendedData;
};

class QtTestMetaDataService : public QMediaService
{
    Q_OBJECT
public:
    QtTestMetaDataService(QObject *parent = 0):QMediaService(parent), metaDataRef(0), hasMetaData(true)
    {
    }

    QMediaControl *requestControl(const char *iid)
    {
        if (hasMetaData && qstrcmp(iid, QMetaDataReaderControl_iid) == 0)
            return &metaData;
        else
            return 0;
    }

    void releaseControl(QMediaControl *)
    {
    }

    QtTestMetaDataProvider metaData;
    int metaDataRef;
    bool hasMetaData;
};

class QtTestMediaObject : public QMediaObject
{
    Q_OBJECT
    Q_PROPERTY(int a READ a WRITE setA NOTIFY aChanged)
    Q_PROPERTY(int b READ b WRITE setB NOTIFY bChanged)
    Q_PROPERTY(int c READ c WRITE setC NOTIFY cChanged)
    Q_PROPERTY(int d READ d WRITE setD)
public:
    QtTestMediaObject(QMediaService *service = 0): QMediaObject(0, service), m_a(0), m_b(0), m_c(0), m_d(0) {}

    using QMediaObject::addPropertyWatch;
    using QMediaObject::removePropertyWatch;

    int a() const { return m_a; }
    void setA(int a) { m_a = a; }

    int b() const { return m_b; }
    void setB(int b) { m_b = b; }

    int c() const { return m_c; }
    void setC(int c) { m_c = c; }

    int d() const { return m_d; }
    void setD(int d) { m_d = d; }

Q_SIGNALS:
    void aChanged(int a);
    void bChanged(int b);
    void cChanged(int c);

private:
    int m_a;
    int m_b;
    int m_c;
    int m_d;
};
#endif //TST_QMEDIAOBJECT_H