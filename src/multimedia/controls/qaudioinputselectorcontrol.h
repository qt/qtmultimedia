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

#ifndef QAUDIOINPUTSELECTORCONTROL_H
#define QAUDIOINPUTSELECTORCONTROL_H

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qmediacontrol.h>

QT_BEGIN_NAMESPACE


// Class forward declaration required for QDoc bug
class QString;
class Q_MULTIMEDIA_EXPORT QAudioInputSelectorControl : public QMediaControl
{
    Q_OBJECT

public:
    virtual ~QAudioInputSelectorControl();

    virtual QList<QString> availableInputs() const = 0;
    virtual QString inputDescription(const QString& name) const = 0;
    virtual QString defaultInput() const = 0;
    virtual QString activeInput() const = 0;

public Q_SLOTS:
    virtual void setActiveInput(const QString& name) = 0;

Q_SIGNALS:
    void activeInputChanged(const QString& name);
    void availableInputsChanged();

protected:
    explicit QAudioInputSelectorControl(QObject *parent = nullptr);
};

#define QAudioInputSelectorControl_iid "org.qt-project.qt.audioinputselectorcontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QAudioInputSelectorControl, QAudioInputSelectorControl_iid)

QT_END_NAMESPACE


#endif // QAUDIOINPUTSELECTORCONTROL_H
