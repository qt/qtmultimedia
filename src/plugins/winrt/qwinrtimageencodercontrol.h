/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
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

#ifndef QWINRTIMAGEENCODERCONTROL_H
#define QWINRTIMAGEENCODERCONTROL_H

#include <qimageencodercontrol.h>

QT_BEGIN_NAMESPACE

class QWinRTImageEncoderControlPrivate;
class QWinRTImageEncoderControl : public QImageEncoderControl
{
    Q_OBJECT
public:
    explicit QWinRTImageEncoderControl(QObject *parent = nullptr);

    QStringList supportedImageCodecs() const override;
    QString imageCodecDescription(const QString &codecName) const override;
    QList<QSize> supportedResolutions(const QImageEncoderSettings &settings, bool *continuous = nullptr) const override;
    QImageEncoderSettings imageSettings() const override;
    void setImageSettings(const QImageEncoderSettings &settings) override;

    void setSupportedResolutionsList(const QList<QSize> resolution);
    void applySettings();

private:
    QScopedPointer<QWinRTImageEncoderControlPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTImageEncoderControl)
};

QT_END_NAMESPACE

#endif // QWINRTIMAGEENCODERCONTROL_H
