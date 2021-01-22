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

#ifndef QSOUNDBUFFER_P_H
#define QSOUNDBUFFER_P_H

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

#include <QObject>

QT_BEGIN_NAMESPACE

class QSoundBuffer : public QObject
{
    Q_OBJECT

public:
    enum State
    {
        Creating,
        Loading,
        Error,
        Ready
    };

    virtual State state() const = 0;

    virtual void load() = 0;

Q_SIGNALS:
    void stateChanged(State state);
    void ready();
    void error();

protected:
    QSoundBuffer(QObject *parent);
};

QT_END_NAMESPACE

#endif // QSOUNDBUFFER_P_H
