/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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

#ifndef QSOUND_H
#define QSOUND_H

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QSoundEffect;

class Q_MULTIMEDIA_EXPORT QSound : public QObject
{
    Q_OBJECT
public:
    enum Loop
    {
        Infinite = -1
    };

    static void play(const QString &filename);

    explicit QSound(const QString &filename, QObject *parent = nullptr);
    ~QSound();

    int loops() const;
    int loopsRemaining() const;
    void setLoops(int);
    QString fileName() const;

    bool isFinished() const;

public Q_SLOTS:
    void play();
    void stop();

private Q_SLOTS:
    void deleteOnComplete();

private:
    QSoundEffect *m_soundEffect = nullptr;
};

QT_END_NAMESPACE


#endif // QSOUND_H
