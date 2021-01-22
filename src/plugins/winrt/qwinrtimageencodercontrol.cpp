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

#include "qwinrtimageencodercontrol.h"
QT_BEGIN_NAMESPACE

class QWinRTImageEncoderControlPrivate
{
public:
    QList<QSize> supportedResolutions;
    QImageEncoderSettings imageEncoderSetting;
};

QWinRTImageEncoderControl::QWinRTImageEncoderControl(QObject *parent)
    : QImageEncoderControl(parent), d_ptr(new QWinRTImageEncoderControlPrivate)
{
}

QStringList QWinRTImageEncoderControl::supportedImageCodecs() const
{
    return QStringList() << QStringLiteral("jpeg");
}

QString QWinRTImageEncoderControl::imageCodecDescription(const QString &codecName) const
{
    if (codecName == QStringLiteral("jpeg"))
        return tr("JPEG image");

    return QString();
}

QList<QSize> QWinRTImageEncoderControl::supportedResolutions(const QImageEncoderSettings &settings, bool *continuous) const
{
    Q_UNUSED(settings);
    Q_D(const QWinRTImageEncoderControl);

    if (continuous)
        *continuous = false;

    return d->supportedResolutions;
}

QImageEncoderSettings QWinRTImageEncoderControl::imageSettings() const
{
    Q_D(const QWinRTImageEncoderControl);
    return d->imageEncoderSetting;
}

void QWinRTImageEncoderControl::setImageSettings(const QImageEncoderSettings &settings)
{
    Q_D(QWinRTImageEncoderControl);
    if (d->imageEncoderSetting == settings)
        return;

    d->imageEncoderSetting = settings;
    applySettings();
}

void QWinRTImageEncoderControl::setSupportedResolutionsList(const QList<QSize> resolution)
{
    Q_D(QWinRTImageEncoderControl);
    d->supportedResolutions = resolution;
    applySettings();
}

void QWinRTImageEncoderControl::applySettings()
{
    Q_D(QWinRTImageEncoderControl);
    if (d->imageEncoderSetting.codec().isEmpty())
        d->imageEncoderSetting.setCodec(QStringLiteral("jpeg"));

    if (d->supportedResolutions.isEmpty())
        return;

    QSize requestResolution = d->imageEncoderSetting.resolution();
    if (!requestResolution.isValid()) {
        d->imageEncoderSetting.setResolution(d->supportedResolutions.last());
        return;
    }
    if (d->supportedResolutions.contains(requestResolution))
        return;

    // Find closest resolution from the list
    const int pixelCount = requestResolution.width() * requestResolution.height();
    int minimumGap = std::numeric_limits<int>::max();
    for (const QSize &size : qAsConst(d->supportedResolutions)) {
        int gap = qAbs(pixelCount - size.width() * size.height());
        if (gap < minimumGap) {
            minimumGap = gap;
            requestResolution = size;
            if (gap == 0)
                break;
        }
    }
    d->imageEncoderSetting.setResolution(requestResolution);
}

QT_END_NAMESPACE
