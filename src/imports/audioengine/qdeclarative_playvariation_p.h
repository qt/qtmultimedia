/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEPLAYVARIATION_P_H
#define QDECLARATIVEPLAYVARIATION_P_H

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

class QDeclarativeAudioSample;
class QSoundInstance;
class QDeclarativeAudioEngine;

class QDeclarativePlayVariation : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString sample READ sample WRITE setSample)
    Q_PROPERTY(bool looping READ isLooping WRITE setLooping)
    Q_PROPERTY(qreal maxGain READ maxGain WRITE setMaxGain)
    Q_PROPERTY(qreal minGain READ minGain WRITE setMinGain)
    Q_PROPERTY(qreal maxPitch READ maxPitch WRITE setMaxPitch)
    Q_PROPERTY(qreal minPitch READ minPitch WRITE setMinPitch)

public:
    QDeclarativePlayVariation(QObject *parent = 0);
    ~QDeclarativePlayVariation();

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

    void setEngine(QDeclarativeAudioEngine *engine);

private:
    Q_DISABLE_COPY(QDeclarativePlayVariation);
    QString m_sample;
    bool m_looping;
    qreal m_maxGain;
    qreal m_minGain;
    qreal m_maxPitch;
    qreal m_minPitch;
    QDeclarativeAudioSample *m_sampleObject;
    QDeclarativeAudioEngine *m_engine;
};

QT_END_NAMESPACE

#endif
