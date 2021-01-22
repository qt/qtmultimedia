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

#ifndef QCAMERAIMAGEPROCESSING_H
#define QCAMERAIMAGEPROCESSING_H

#include <QtCore/qstringlist.h>
#include <QtCore/qpair.h>
#include <QtCore/qsize.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qmediaobject.h>
#include <QtMultimedia/qmediaservice.h>
#include <QtMultimedia/qmediaenumdebug.h>

QT_BEGIN_NAMESPACE


class QCamera;

class QCameraImageProcessingPrivate;
class Q_MULTIMEDIA_EXPORT QCameraImageProcessing : public QObject
{
    Q_OBJECT
    Q_ENUMS(WhiteBalanceMode ColorFilter)
public:
    enum WhiteBalanceMode {
        WhiteBalanceAuto = 0,
        WhiteBalanceManual = 1,
        WhiteBalanceSunlight = 2,
        WhiteBalanceCloudy = 3,
        WhiteBalanceShade = 4,
        WhiteBalanceTungsten = 5,
        WhiteBalanceFluorescent = 6,
        WhiteBalanceFlash = 7,
        WhiteBalanceSunset = 8,
        WhiteBalanceVendor = 1000
    };

    enum ColorFilter {
        ColorFilterNone,
        ColorFilterGrayscale,
        ColorFilterNegative,
        ColorFilterSolarize,
        ColorFilterSepia,
        ColorFilterPosterize,
        ColorFilterWhiteboard,
        ColorFilterBlackboard,
        ColorFilterAqua,
        ColorFilterVendor = 1000
    };

    bool isAvailable() const;

    WhiteBalanceMode whiteBalanceMode() const;
    void setWhiteBalanceMode(WhiteBalanceMode mode);
    bool isWhiteBalanceModeSupported(WhiteBalanceMode mode) const;

    qreal manualWhiteBalance() const;
    void setManualWhiteBalance(qreal colorTemperature);

    qreal brightness() const;
    void setBrightness(qreal value);

    qreal contrast() const;
    void setContrast(qreal value);

    qreal saturation() const;
    void setSaturation(qreal value);

    qreal sharpeningLevel() const;
    void setSharpeningLevel(qreal value);

    qreal denoisingLevel() const;
    void setDenoisingLevel(qreal value);

    ColorFilter colorFilter() const;
    void setColorFilter(ColorFilter filter);
    bool isColorFilterSupported(ColorFilter filter) const;

protected:
    ~QCameraImageProcessing();

private:
    friend class QCamera;
    friend class QCameraPrivate;
    QCameraImageProcessing(QCamera *camera);

    Q_DISABLE_COPY(QCameraImageProcessing)
    Q_DECLARE_PRIVATE(QCameraImageProcessing)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCameraImageProcessingPrivate *d_ptr_deprecated;
#endif
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QCameraImageProcessing::WhiteBalanceMode)
Q_DECLARE_METATYPE(QCameraImageProcessing::ColorFilter)

Q_MEDIA_ENUM_DEBUG(QCameraImageProcessing, WhiteBalanceMode)
Q_MEDIA_ENUM_DEBUG(QCameraImageProcessing, ColorFilter)

#endif  // QCAMERAIMAGEPROCESSING_H
