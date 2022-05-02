/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
******************************************************************************/
#ifndef BBCAMERAAUDIOENCODERSETTINGSCONTROL_H
#define BBCAMERAAUDIOENCODERSETTINGSCONTROL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qaudioencodersettingscontrol.h>

QT_BEGIN_NAMESPACE

class BbCameraSession;

class BbCameraAudioEncoderSettingsControl : public QAudioEncoderSettingsControl
{
    Q_OBJECT
public:
    explicit BbCameraAudioEncoderSettingsControl(BbCameraSession *session, QObject *parent = 0);

    QStringList supportedAudioCodecs() const override;
    QString codecDescription(const QString &codecName) const override;
    QList<int> supportedSampleRates(const QAudioEncoderSettings &settings, bool *continuous = 0) const override;
    QAudioEncoderSettings audioSettings() const override;
    void setAudioSettings(const QAudioEncoderSettings &settings) override;

private:
    BbCameraSession *m_session;
};

QT_END_NAMESPACE

#endif
