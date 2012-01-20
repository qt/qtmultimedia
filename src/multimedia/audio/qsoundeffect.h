/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSOUNDEFFECT_H
#define QSOUNDEFFECT_H

#include <qtmultimediadefs.h>
#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qstringlist.h>


QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Multimedia)


class QSoundEffectPrivate;

class Q_MULTIMEDIA_EXPORT QSoundEffect : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("DefaultMethod", "play()")
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int loops READ loopCount WRITE setLoopCount NOTIFY loopCountChanged)
    Q_PROPERTY(int loopsRemaining READ loopsRemaining NOTIFY loopsRemainingChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_ENUMS(Loop)
    Q_ENUMS(Status)

public:
    enum Loop
    {
        Infinite = -2
    };

    enum Status
    {
        Null,
        Loading,
        Ready,
        Error
    };

    explicit QSoundEffect(QObject *parent = 0);
    ~QSoundEffect();

    static QStringList supportedMimeTypes();

    QUrl source() const;
    void setSource(const QUrl &url);

    int loopCount() const;
    int loopsRemaining() const;
    void setLoopCount(int loopCount);

    qreal volume() const;
    void setVolume(qreal volume);

    bool isMuted() const;
    void setMuted(bool muted);

    bool isLoaded() const;

    bool isPlaying() const;
    Status status() const;

Q_SIGNALS:
    void sourceChanged();
    void loopCountChanged();
    void loopsRemainingChanged();
    void volumeChanged();
    void mutedChanged();
    void loadedChanged();
    void playingChanged();
    void statusChanged();

public Q_SLOTS:
    void play();
    void stop();

private:
    Q_DISABLE_COPY(QSoundEffect)
    QSoundEffectPrivate* d;
};

QT_END_NAMESPACE

QT_END_HEADER


#endif // QSOUNDEFFECT_H
