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

#ifndef QVIDEOENCODERSETTINGSCONTROL_H
#define QVIDEOENCODERSETTINGSCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qmediarecorder.h>

#include <QtCore/qpair.h>
#include <QtCore/qsize.h>

QT_BEGIN_NAMESPACE

class QByteArray;
class QStringList;
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QVideoEncoderSettingsControl : public QMediaControl
{
    Q_OBJECT

public:
    virtual ~QVideoEncoderSettingsControl();

    virtual QList<QSize> supportedResolutions(const QVideoEncoderSettings &settings,
                                              bool *continuous = nullptr) const = 0;

    virtual QList<qreal> supportedFrameRates(const QVideoEncoderSettings &settings,
                                             bool *continuous = nullptr) const = 0;

    virtual QStringList supportedVideoCodecs() const = 0;
    virtual QString videoCodecDescription(const QString &codec) const = 0;

    virtual QVideoEncoderSettings videoSettings() const = 0;
    virtual void setVideoSettings(const QVideoEncoderSettings &settings) = 0;

protected:
    explicit QVideoEncoderSettingsControl(QObject *parent = nullptr);
};

#define QVideoEncoderSettingsControl_iid "org.qt-project.qt.videoencodersettingscontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QVideoEncoderSettingsControl, QVideoEncoderSettingsControl_iid)

QT_END_NAMESPACE


#endif
