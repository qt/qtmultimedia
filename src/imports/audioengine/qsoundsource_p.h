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

#ifndef QSOUNDSOURCE_P_H
#define QSOUNDSOURCE_P_H

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

#include <QVector3D>
#include <QObject>

QT_BEGIN_NAMESPACE

class QSoundBuffer;

class QSoundSource : public QObject
{
    Q_OBJECT
public:
    enum State
    {
        StoppedState,
        PlayingState,
        PausedState
    };

    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;

    virtual QSoundSource::State state() const = 0;

    virtual void setLooping(bool looping) = 0;
    virtual void setDirection(const QVector3D& direction) = 0;
    virtual void setPosition(const QVector3D& position) = 0;
    virtual void setVelocity(const QVector3D& velocity) = 0;

    virtual QVector3D velocity() const = 0;
    virtual QVector3D position() const = 0;
    virtual QVector3D direction() const = 0;

    virtual void setGain(qreal gain) = 0;
    virtual void setPitch(qreal pitch) = 0;
    virtual void setCone(qreal innerAngle, qreal outerAngle, qreal outerGain) = 0;

    virtual void bindBuffer(QSoundBuffer*) = 0;
    virtual void unbindBuffer() = 0;

Q_SIGNALS:
    void stateChanged(QSoundSource::State newState);

protected:
    QSoundSource(QObject *parent);
};

QT_END_NAMESPACE

#endif
