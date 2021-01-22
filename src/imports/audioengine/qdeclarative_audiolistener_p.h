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

#ifndef QDECLARATIVEAUDIOLISTENER_P_H
#define QDECLARATIVEAUDIOLISTENER_P_H

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

#include <QtCore/QObject>
#include <QtGui/qvector3d.h>

QT_BEGIN_NAMESPACE

class QDeclarativeAudioEngine;

class QDeclarativeAudioListener : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDeclarativeAudioEngine* engine READ engine WRITE setEngine)
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(QVector3D direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(QVector3D velocity READ velocity WRITE setVelocity NOTIFY velocityChanged)
    Q_PROPERTY(QVector3D up READ up WRITE setUp NOTIFY upChanged)
    Q_PROPERTY(qreal gain READ gain WRITE setGain NOTIFY gainChanged)

public:
    QDeclarativeAudioListener(QObject *parent = 0);
    ~QDeclarativeAudioListener();

    QDeclarativeAudioEngine* engine() const;
    void setEngine(QDeclarativeAudioEngine *engine);

    QVector3D position() const;
    void setPosition(const QVector3D &position);

    QVector3D direction() const;
    void setDirection(const QVector3D &direction);

    QVector3D up() const;
    void setUp(const QVector3D &up);

    QVector3D velocity() const;
    void setVelocity(const QVector3D &velocity);

    qreal gain() const;
    void setGain(qreal gain);

Q_SIGNALS:
    void positionChanged();
    void directionChanged();
    void velocityChanged();
    void upChanged();
    void gainChanged();

private:
    Q_DISABLE_COPY(QDeclarativeAudioListener);
    QDeclarativeAudioEngine *m_engine;
};

QT_END_NAMESPACE

#endif
