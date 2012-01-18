/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEPLAYVARIATION_P_H
#define QDECLARATIVEPLAYVARIATION_P_H

#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qdeclarativecomponent.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QDeclarativeAudioSample;
class QSoundInstance;

class QDeclarativePlayVariation : public QObject, public QDeclarativeParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QDeclarativeParserStatus)
    Q_PROPERTY(QString sample READ sample WRITE setSample)
    Q_PROPERTY(bool looping READ isLooping WRITE setLooping)
    Q_PROPERTY(qreal maxGain READ maxGain WRITE setMaxGain)
    Q_PROPERTY(qreal minGain READ minGain WRITE setMinGain)
    Q_PROPERTY(qreal maxPitch READ maxPitch WRITE setMaxPitch)
    Q_PROPERTY(qreal minPitch READ minPitch WRITE setMinPitch)

public:
    QDeclarativePlayVariation(QObject *parent = 0);
    ~QDeclarativePlayVariation();

    void classBegin();
    void componentComplete();

    QString sample() const;
    void setSample(const QString& sample);

    bool isLooping() const;
    void setLooping(bool looping);

    qreal maxGain() const;
    void setMaxGain(qreal maxGain);
    qreal minGain() const;
    void setMinGain(qreal minGain);

    qreal maxPitch() const;
    void setMaxPitch(qreal maxPitch);
    qreal minPitch() const;
    void setMinPitch(qreal minPitch);

    //called by QDeclarativeAudioEngine
    void setSampleObject(QDeclarativeAudioSample *sampleObject);
    QDeclarativeAudioSample* sampleObject() const;

    void applyParameters(QSoundInstance *soundInstance);

private:
    Q_DISABLE_COPY(QDeclarativePlayVariation);
    bool m_complete;
    QString m_sample;
    bool m_looping;
    qreal m_maxGain;
    qreal m_minGain;
    qreal m_maxPitch;
    qreal m_minPitch;
    QDeclarativeAudioSample *m_sampleObject;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
