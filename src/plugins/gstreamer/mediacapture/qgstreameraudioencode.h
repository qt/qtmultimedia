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

#ifndef QGSTREAMERAUDIOENCODE_H
#define QGSTREAMERAUDIOENCODE_H

#include <qaudioencodersettingscontrol.h>

#include <QtCore/qstringlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qset.h>

#include <gst/gst.h>

#include <qaudioformat.h>
#include <private/qgstcodecsinfo_p.h>

QT_BEGIN_NAMESPACE

class QGstreamerCaptureSession;

class QGstreamerAudioEncode : public QAudioEncoderSettingsControl
{
    Q_OBJECT
public:
    QGstreamerAudioEncode(QObject *parent);
    virtual ~QGstreamerAudioEncode();

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

    GstElement *createEncoder();

    QSet<QString> supportedStreamTypes(const QString &codecName) const;

private:
    QGstCodecsInfo m_codecs;

    QMap<QString, QMap<QString, QVariant> > m_options;

    QAudioEncoderSettings m_audioSettings;
};

QT_END_NAMESPACE

#endif
