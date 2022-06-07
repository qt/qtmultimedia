// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "qpushbutton.h"
#include "qsoundeffect.h"

void qsoundeffectsnippet() {
    //! [2]
    QSoundEffect effect;
    effect.setSource(QUrl::fromLocalFile("engine.wav"));
    effect.setLoopCount(QSoundEffect::Infinite);
    effect.setVolume(0.25f);
    effect.play();
    //! [2]
}

QPushButton *clickSource;

class MyGame : public QObject {
    Q_OBJECT
public:
    //! [3]
    MyGame()
        : m_explosion(this)
    {
        m_explosion.setSource(QUrl::fromLocalFile("explosion.wav"));
        m_explosion.setVolume(0.25f);

        // Set up click handling etc.
        connect(clickSource, &QPushButton::clicked, &m_explosion, &QSoundEffect::play);
    }
private:
    QSoundEffect m_explosion;
    //! [3]
};
