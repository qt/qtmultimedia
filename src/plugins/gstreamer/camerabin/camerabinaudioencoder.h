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

#ifndef CAMERABINAUDIOENCODE_H
#define CAMERABINAUDIOENCODE_H

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <qaudioencodersettingscontrol.h>

#include <QtCore/qstringlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qset.h>

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#if QT_CONFIG(gstreamer_encodingprofiles)
#include <gst/pbutils/encoding-profile.h>
#include <private/qgstcodecsinfo_p.h>
#endif

#include <qaudioformat.h>

QT_BEGIN_NAMESPACE
class CameraBinSession;

class CameraBinAudioEncoder : public QAudioEncoderSettingsControl
{
    Q_OBJECT
public:
    CameraBinAudioEncoder(QObject *parent);
    virtual ~CameraBinAudioEncoder();

    QStringList supportedAudioCodecs() const override;
    QString codecDescription(const QString &codecName) const override;

    QStringList supportedEncodingOptions(const QString &codec) const;
    QVariant encodingOption(const QString &codec, const QString &name) const;
    void setEncodingOption(const QString &codec, const QString &name, const QVariant &value);

    QList<int> supportedSampleRates(const QAudioEncoderSettings &settings = QAudioEncoderSettings(),
                                    bool *isContinuous = 0) const override;
    QList<int> supportedChannelCounts(const QAudioEncoderSettings &settings = QAudioEncoderSettings()) const;
    QList<int> supportedSampleSizes(const QAudioEncoderSettings &settings = QAudioEncoderSettings()) const;

    QAudioEncoderSettings audioSettings() const override;
    void setAudioSettings(const QAudioEncoderSettings &) override;

    QAudioEncoderSettings actualAudioSettings() const;
    void setActualAudioSettings(const QAudioEncoderSettings&);
    void resetActualSettings();

#if QT_CONFIG(gstreamer_encodingprofiles)
    GstEncodingProfile *createProfile();
#endif

    void applySettings(GstElement *element);

Q_SIGNALS:
    void settingsChanged();

private:
#if QT_CONFIG(gstreamer_encodingprofiles)
    QGstCodecsInfo m_codecs;
#endif

    QAudioEncoderSettings m_actualAudioSettings;
    QAudioEncoderSettings m_audioSettings;
};

QT_END_NAMESPACE

#endif
