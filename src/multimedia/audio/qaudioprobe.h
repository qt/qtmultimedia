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

#ifndef QAUDIOPROBE_H
#define QAUDIOPROBE_H

#include <QtCore/qobject.h>
#include <QtMultimedia/qaudiobuffer.h>

QT_BEGIN_NAMESPACE

class QMediaObject;
class QMediaRecorder;

class QAudioProbePrivate;
class Q_MULTIMEDIA_EXPORT QAudioProbe : public QObject
{
    Q_OBJECT
public:
    explicit QAudioProbe(QObject *parent = nullptr);
    ~QAudioProbe();

    bool setSource(QMediaObject *source);
    bool setSource(QMediaRecorder *source);

    bool isActive() const;

Q_SIGNALS:
    void audioBufferProbed(const QAudioBuffer &buffer);
    void flush();

private:
    QAudioProbePrivate *d;
};

QT_END_NAMESPACE

#endif // QAUDIOPROBE_H
