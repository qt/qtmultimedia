// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#ifndef QQUICK3DAMBIENTSOUND_H
#define QQUICK3DAMBIENTSOUND_H

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

#include <private/qquick3dnode_p.h>
#include <QUrl>
#include <qvector3d.h>

QT_BEGIN_NAMESPACE

class QAmbientSound;

class QQuick3DAmbientSound : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(int loops READ loops WRITE setLoops NOTIFY loopsChanged)
    Q_PROPERTY(bool autoPlay READ autoPlay WRITE setAutoPlay NOTIFY autoPlayChanged)
    QML_NAMED_ELEMENT(AmbientSound)

public:
    QQuick3DAmbientSound();
    ~QQuick3DAmbientSound() override;

    void setSource(QUrl source);
    QUrl source() const;

    void setVolume(float volume);
    float volume() const;

    enum Loops
    {
        Infinite = -1,
        Once = 1
    };
    Q_ENUM(Loops)

    int loops() const;
    void setLoops(int loops);

    bool autoPlay() const;
    void setAutoPlay(bool autoPlay);

public Q_SLOTS:
    void play();
    void pause();
    void stop();

Q_SIGNALS:
    void sourceChanged();
    void volumeChanged();
    void loopsChanged();
    void autoPlayChanged();

private:
    QAmbientSound *m_sound = nullptr;
};

QT_END_NAMESPACE

#endif
