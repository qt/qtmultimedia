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

#ifndef QDECLARATIVEATTENUATIONMODEL_P_H
#define QDECLARATIVEATTENUATIONMODEL_P_H

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
#include <QVector3D>

QT_BEGIN_NAMESPACE

class QDeclarativeAudioEngine;

class QDeclarativeAttenuationModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName)

public:
    QDeclarativeAttenuationModel(QObject *parent = 0);
    ~QDeclarativeAttenuationModel();

    QString name() const;
    void setName(const QString& name);

    virtual qreal calculateGain(const QVector3D &listenerPosition, const QVector3D &sourcePosition) const = 0;

    virtual void setEngine(QDeclarativeAudioEngine *engine);

protected:
    QString m_name;
    QDeclarativeAudioEngine *m_engine;

private:
    Q_DISABLE_COPY(QDeclarativeAttenuationModel);
};

class QDeclarativeAttenuationModelLinear : public QDeclarativeAttenuationModel
{
    Q_OBJECT
    Q_PROPERTY(qreal start READ startDistance WRITE setStartDistance)
    Q_PROPERTY(qreal end READ endDistance WRITE setEndDistance)

public:
    QDeclarativeAttenuationModelLinear(QObject *parent = 0);

    qreal startDistance() const;
    void setStartDistance(qreal startDist);

    qreal endDistance() const;
    void setEndDistance(qreal endDist);

    qreal calculateGain(const QVector3D &listenerPosition, const QVector3D &sourcePosition) const override;

    void setEngine(QDeclarativeAudioEngine *engine) override;

private:
    Q_DISABLE_COPY(QDeclarativeAttenuationModelLinear);
    qreal m_start;
    qreal m_end;
};

class QDeclarativeAttenuationModelInverse : public QDeclarativeAttenuationModel
{
    Q_OBJECT
    Q_PROPERTY(qreal start READ referenceDistance WRITE setReferenceDistance)
    Q_PROPERTY(qreal end READ maxDistance WRITE setMaxDistance)
    Q_PROPERTY(qreal rolloff READ rolloffFactor WRITE setRolloffFactor)

public:
    QDeclarativeAttenuationModelInverse(QObject *parent = 0);

    qreal referenceDistance() const;
    void setReferenceDistance(qreal referenceDistance);

    qreal maxDistance() const;
    void setMaxDistance(qreal maxDistance);

    qreal rolloffFactor() const;
    void setRolloffFactor(qreal rolloffFactor);

    qreal calculateGain(const QVector3D &listenerPosition, const QVector3D &sourcePosition) const override;

    void setEngine(QDeclarativeAudioEngine *engine) override;

private:
    Q_DISABLE_COPY(QDeclarativeAttenuationModelInverse);
    qreal m_ref;
    qreal m_max;
    qreal m_rolloff;
};

QT_END_NAMESPACE

#endif
