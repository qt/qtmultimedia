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

#ifndef QVIDEOPROBE_H
#define QVIDEOPROBE_H

#include <QtCore/QObject>
#include <QtMultimedia/qvideoframe.h>

QT_BEGIN_NAMESPACE

class QMediaObject;
class QMediaRecorder;

class QVideoProbePrivate;
class Q_MULTIMEDIA_EXPORT QVideoProbe : public QObject
{
    Q_OBJECT
public:
    explicit QVideoProbe(QObject *parent = nullptr);
    ~QVideoProbe();

    bool setSource(QMediaObject *source);
    bool setSource(QMediaRecorder *source);

    bool isActive() const;

Q_SIGNALS:
    void videoFrameProbed(const QVideoFrame &frame);
    void flush();

private:
    QVideoProbePrivate *d;
};

QT_END_NAMESPACE

#endif // QVIDEOPROBE_H
