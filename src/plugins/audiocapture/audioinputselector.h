/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef AUDIOINPUTSELECTOR_H
#define AUDIOINPUTSELECTOR_H

#include <QStringList>

#include "qaudioinputselectorcontrol.h"

class AudioCaptureSession;

QT_USE_NAMESPACE

class AudioInputSelector : public QAudioInputSelectorControl
{
Q_OBJECT
public:
    AudioInputSelector(QObject *parent);
    virtual ~AudioInputSelector();

    QList<QString> availableInputs() const;
    QString inputDescription(const QString& name) const;
    QString defaultInput() const;
    QString activeInput() const;

public Q_SLOTS:
    void setActiveInput(const QString& name);

private:
    void update();

    QString        m_audioInput;
    QList<QString> m_names;
    QList<QString> m_descriptions;
    AudioCaptureSession* m_session;
};

#endif // AUDIOINPUTSELECTOR_H
