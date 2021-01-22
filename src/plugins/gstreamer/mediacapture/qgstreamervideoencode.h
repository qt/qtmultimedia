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

#ifndef QGSTREAMERVIDEOENCODE_H
#define QGSTREAMERVIDEOENCODE_H

#include <qvideoencodersettingscontrol.h>

#include <QtCore/qstringlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qset.h>

#include <private/qgstcodecsinfo_p.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE

class QGstreamerCaptureSession;

class QGstreamerVideoEncode : public QVideoEncoderSettingsControl
{
    Q_OBJECT
public:
    QGstreamerVideoEncode(QGstreamerCaptureSession *session);
    virtual ~QGstreamerVideoEncode();

    QList<QSize> supportedResolutions(const QVideoEncoderSettings &settings = QVideoEncoderSettings(),
                                      bool *continuous = 0) const override;

    QList< qreal > supportedFrameRates(const QVideoEncoderSettings &settings = QVideoEncoderSettings(),
                                       bool *continuous = 0) const override;

    QPair<int,int> rateAsRational() const;

    QStringList supportedVideoCodecs() const override;
    QString videoCodecDescription(const QString &codecName) const override;

    QVideoEncoderSettings videoSettings() const override;
    void setVideoSettings(const QVideoEncoderSettings &settings) override;

    QStringList supportedEncodingOptions(const QString &codec) const;
    QVariant encodingOption(const QString &codec, const QString &name) const;
    void setEncodingOption(const QString &codec, const QString &name, const QVariant &value);

    GstElement *createEncoder();

    QSet<QString> supportedStreamTypes(const QString &codecName) const;

private:
    QGstreamerCaptureSession *m_session;

    QGstCodecsInfo m_codecs;

    QVideoEncoderSettings m_videoSettings;
    QMap<QString, QMap<QString, QVariant> > m_options;
};

QT_END_NAMESPACE

#endif
