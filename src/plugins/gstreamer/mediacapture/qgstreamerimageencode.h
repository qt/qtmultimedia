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

#ifndef QGSTREAMERIMAGEENCODE_H
#define QGSTREAMERIMAGEENCODE_H

#include <qimageencodercontrol.h>

#include <QtCore/qstringlist.h>
#include <QtCore/qmap.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE

class QGstreamerCaptureSession;

class QGstreamerImageEncode : public QImageEncoderControl
{
    Q_OBJECT
public:
    QGstreamerImageEncode(QGstreamerCaptureSession *session);
    virtual ~QGstreamerImageEncode();

    QList<QSize> supportedResolutions(const QImageEncoderSettings &settings = QImageEncoderSettings(),
                                      bool *continuous = 0) const override;

    QStringList supportedImageCodecs() const override;
    QString imageCodecDescription(const QString &codecName) const override;

    QImageEncoderSettings imageSettings() const override;
    void setImageSettings(const QImageEncoderSettings &settings) override;

Q_SIGNALS:
    void settingsChanged();

private:
    QImageEncoderSettings m_settings;

    QGstreamerCaptureSession *m_session;
};

QT_END_NAMESPACE

#endif
