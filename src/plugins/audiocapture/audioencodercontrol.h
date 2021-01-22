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

#ifndef AUDIOENCODERCONTROL_H
#define AUDIOENCODERCONTROL_H

#include "qaudioencodersettingscontrol.h"

#include <QtCore/qstringlist.h>
#include <QtCore/qmap.h>

#include <qaudioformat.h>

QT_BEGIN_NAMESPACE

class AudioCaptureSession;

class AudioEncoderControl : public QAudioEncoderSettingsControl
{
    Q_OBJECT
public:
    AudioEncoderControl(QObject *parent);
    virtual ~AudioEncoderControl();

    QStringList supportedAudioCodecs() const;
    QString codecDescription(const QString &codecName) const;
    QList<int> supportedSampleRates(const QAudioEncoderSettings &, bool *continuous = 0) const;

    QAudioEncoderSettings audioSettings() const;
    void setAudioSettings(const QAudioEncoderSettings&);

private:
    void update();

    AudioCaptureSession* m_session;
    QList<int> m_sampleRates;
};

QT_END_NAMESPACE

#endif
