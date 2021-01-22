/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QDECLARATIVEAUDIOCATEGORY_P_H
#define QDECLARATIVEAUDIOCATEGORY_P_H

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

#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QDeclarativeAudioEngine;

class QDeclarativeAudioCategory : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(QString name READ name WRITE setName)

public:
    QDeclarativeAudioCategory(QObject *parent = 0);
    ~QDeclarativeAudioCategory();

    qreal volume() const;
    void setVolume(qreal volume);

    QString name() const;
    void setName(const QString& name);

    void setEngine(QDeclarativeAudioEngine *engine);

Q_SIGNALS:
    void volumeChanged(qreal newVolume);
    void stopped();
    void paused();
    void resumed();

public Q_SLOTS:
    void stop();
    void pause();
    void resume();

private:
    Q_DISABLE_COPY(QDeclarativeAudioCategory);
    QString m_name;
    qreal m_volume;
    QDeclarativeAudioEngine *m_engine;
};

QT_END_NAMESPACE

#endif
