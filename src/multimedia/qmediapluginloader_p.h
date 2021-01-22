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

#ifndef QMEDIAPLUGINLOADER_H
#define QMEDIAPLUGINLOADER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qtmultimediaglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qjsonobject.h>


QT_BEGIN_NAMESPACE

class QFactoryLoader;
class QMediaServiceProviderPlugin;

class Q_MULTIMEDIA_EXPORT QMediaPluginLoader
{
public:
    QMediaPluginLoader(const char *iid,
                   const QString &suffix = QString(),
                   Qt::CaseSensitivity = Qt::CaseSensitive);
    ~QMediaPluginLoader();

    QStringList keys() const;
    QObject* instance(QString const &key);
    QList<QObject*> instances(QString const &key);

private:
    void loadMetadata();

    QByteArray  m_iid;
    QString     m_location;
    QMap<QString, QList<QJsonObject> > m_metadata;

    QFactoryLoader *m_factoryLoader;
};

QT_END_NAMESPACE


#endif  // QMEDIAPLUGINLOADER_H
