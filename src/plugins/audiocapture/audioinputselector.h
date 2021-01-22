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

#ifndef AUDIOINPUTSELECTOR_H
#define AUDIOINPUTSELECTOR_H

#include <QStringList>

#include "qaudioinputselectorcontrol.h"

QT_BEGIN_NAMESPACE

class AudioCaptureSession;

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

QT_END_NAMESPACE

#endif // AUDIOINPUTSELECTOR_H
