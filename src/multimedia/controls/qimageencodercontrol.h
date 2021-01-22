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

#ifndef QIMAGEENCODERCONTROL_H
#define QIMAGEENCODERCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qmediarecorder.h>
#include <QtMultimedia/qmediaencodersettings.h>

#include <QtCore/qsize.h>

QT_BEGIN_NAMESPACE

class QByteArray;
class QStringList;
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIA_EXPORT QImageEncoderControl : public QMediaControl
{
    Q_OBJECT

public:
    virtual ~QImageEncoderControl();

    virtual QStringList supportedImageCodecs() const = 0;
    virtual QString imageCodecDescription(const QString &codec) const = 0;

    virtual QList<QSize> supportedResolutions(const QImageEncoderSettings &settings,
                                              bool *continuous = nullptr) const = 0;

    virtual QImageEncoderSettings imageSettings() const = 0;
    virtual void setImageSettings(const QImageEncoderSettings &settings) = 0;

protected:
    explicit QImageEncoderControl(QObject *parent = nullptr);
};

#define QImageEncoderControl_iid "org.qt-project.qt.imageencodercontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QImageEncoderControl, QImageEncoderControl_iid)

QT_END_NAMESPACE


#endif
