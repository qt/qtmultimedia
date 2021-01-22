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

#ifndef QAUDIOENCODERSETTINGSCONTROL_H
#define QAUDIOENCODERSETTINGSCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qmediarecorder.h>
#include <QtCore/qlist.h>
#include <QtCore/qpair.h>

QT_BEGIN_NAMESPACE

class QStringList;
class QAudioFormat;
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QAudioEncoderSettingsControl : public QMediaControl
{
    Q_OBJECT

public:
    virtual ~QAudioEncoderSettingsControl();

    virtual QStringList supportedAudioCodecs() const = 0;
    virtual QString codecDescription(const QString &codecName) const = 0;

    virtual QList<int> supportedSampleRates(const QAudioEncoderSettings &settings,
                                            bool *continuous = nullptr) const = 0;

    virtual QAudioEncoderSettings audioSettings() const = 0;
    virtual void setAudioSettings(const QAudioEncoderSettings &settings) = 0;

protected:
    explicit QAudioEncoderSettingsControl(QObject *parent = nullptr);
};

#define QAudioEncoderSettingsControl_iid "org.qt-project.qt.audioencodersettingscontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QAudioEncoderSettingsControl, QAudioEncoderSettingsControl_iid)

QT_END_NAMESPACE


#endif // QAUDIOENCODERSETTINGSCONTROL_H
