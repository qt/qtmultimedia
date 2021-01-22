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

#ifndef QSOUNDINSTANCE_P_H
#define QSOUNDINSTANCE_P_H

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
#include "qsoundsource_p.h"

QT_BEGIN_NAMESPACE

class QDeclarativeSound;
class QDeclarativeAudioEngine;

class QSoundInstance : public QObject
{
    Q_OBJECT
public:
    explicit QSoundInstance(QObject *parent);
    ~QSoundInstance();

    void play();

    enum State
    {
        StoppedState = QSoundSource::StoppedState,
        PlayingState = QSoundSource::PlayingState,
        PausedState = QSoundSource::PausedState
    };
    State state() const;

    void setPosition(const QVector3D& position);
    void setDirection(const QVector3D& direction);
    void setVelocity(const QVector3D& velocity);

    //this gain and pitch is used for dynamic user control during execution
    void setGain(qreal gain);
    void setPitch(qreal pitch);
    void setCone(qreal innerAngle, qreal outerAngle, qreal outerGain);

    //this varPitch and varGain is calculated from config in PlayVariation
    void updateVariationParameters(qreal varPitch, qreal varGain, bool looping);

    void bindSoundDescription(QDeclarativeSound *sound);

    void update3DVolume(const QVector3D& listenerPosition);

    bool attenuationEnabled() const;

Q_SIGNALS:
    void stateChanged(QSoundInstance::State state);

public Q_SLOTS:
    void pause();
    void stop();

private Q_SLOTS:
    void resume();
    void bufferReady();
    void categoryVolumeChanged();
    void handleSourceStateChanged(QSoundSource::State);

private:
    void setState(State state);
    void prepareNewVariation();
    void detach();
    qreal categoryVolume() const;
    void updatePitch();
    void updateGain();
    void updateConeOuterGain();

    void sourcePlay();
    void sourcePause();
    void sourceStop();

    QSoundSource *m_soundSource;
    QSoundBuffer *m_bindBuffer;

    QDeclarativeSound *m_sound;
    int m_variationIndex;

    bool                 m_isReady; //true if the sound source is already bound to some sound buffer
    qreal                m_gain;
    qreal                m_attenuationGain;
    qreal                m_varGain;
    qreal                m_pitch;
    qreal                m_varPitch;
    State                m_state;
    qreal                m_coneOuterGain;

    QDeclarativeAudioEngine *m_engine;
};

QT_END_NAMESPACE

#endif // QSOUNDINSTANCE_P_H
