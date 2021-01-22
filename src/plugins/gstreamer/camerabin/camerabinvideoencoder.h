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

#ifndef CAMERABINVIDEOENCODE_H
#define CAMERABINVIDEOENCODE_H

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <qvideoencodersettingscontrol.h>

#include <QtCore/qstringlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qset.h>

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#if QT_CONFIG(gstreamer_encodingprofiles)
#include <gst/pbutils/encoding-profile.h>
#include <private/qgstcodecsinfo_p.h>
#endif

QT_BEGIN_NAMESPACE

class CameraBinSession;

class CameraBinVideoEncoder : public QVideoEncoderSettingsControl
{
    Q_OBJECT
public:
    CameraBinVideoEncoder(CameraBinSession *session);
    virtual ~CameraBinVideoEncoder();

    QList<QSize> supportedResolutions(const QVideoEncoderSettings &settings = QVideoEncoderSettings(),
                                      bool *continuous = 0) const override;

    QList< qreal > supportedFrameRates(const QVideoEncoderSettings &settings = QVideoEncoderSettings(),
                                       bool *continuous = 0) const override;

    QPair<int,int> rateAsRational(qreal) const;

    QStringList supportedVideoCodecs() const override;
    QString videoCodecDescription(const QString &codecName) const override;

    QVideoEncoderSettings videoSettings() const override;
    void setVideoSettings(const QVideoEncoderSettings &settings) override;

    QVideoEncoderSettings actualVideoSettings() const;
    void setActualVideoSettings(const QVideoEncoderSettings&);
    void resetActualSettings();

#if QT_CONFIG(gstreamer_encodingprofiles)
    GstEncodingProfile *createProfile();
#endif

    void applySettings(GstElement *encoder);

Q_SIGNALS:
    void settingsChanged();

private:
    CameraBinSession *m_session;

#if QT_CONFIG(gstreamer_encodingprofiles)
    QGstCodecsInfo m_codecs;
#endif

    QVideoEncoderSettings m_actualVideoSettings;
    QVideoEncoderSettings m_videoSettings;
};

QT_END_NAMESPACE

#endif
