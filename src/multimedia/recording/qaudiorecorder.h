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

#ifndef QAUDIORECORDER_H
#define QAUDIORECORDER_H

#include <QtMultimedia/qmediarecorder.h>
#include <QtMultimedia/qmediaobject.h>
#include <QtMultimedia/qmediaencodersettings.h>
#include <QtMultimedia/qmediaenumdebug.h>

#include <QtCore/qpair.h>

QT_BEGIN_NAMESPACE

class QString;
class QSize;
class QAudioFormat;
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE

class QAudioRecorderPrivate;
class Q_MULTIMEDIA_EXPORT QAudioRecorder : public QMediaRecorder
{
    Q_OBJECT
    Q_PROPERTY(QString audioInput READ audioInput WRITE setAudioInput NOTIFY audioInputChanged)
public:
    explicit QAudioRecorder(QObject *parent = nullptr);
    ~QAudioRecorder();

    QStringList audioInputs() const;
    QString defaultAudioInput() const;
    QString audioInputDescription(const QString& name) const;

    QString audioInput() const;

public Q_SLOTS:
    void setAudioInput(const QString& name);

Q_SIGNALS:
    void audioInputChanged(const QString& name);
    void availableAudioInputsChanged();

private:
    Q_DISABLE_COPY(QAudioRecorder)
    Q_DECLARE_PRIVATE(QAudioRecorder)
};

QT_END_NAMESPACE

#endif  // QAUDIORECORDER_H
