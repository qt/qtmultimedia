/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCAMERAIMAGEPROCESSING_H
#define QCAMERAIMAGEPROCESSING_H

#include <QtCore/qstringlist.h>
#include <QtCore/qpair.h>
#include <QtCore/qsize.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtCore/qobject.h>

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qmediaenumdebug.h>

QT_BEGIN_NAMESPACE


class QCamera;
class QPlatformCamera;

class QCameraImageProcessingPrivate;
class Q_MULTIMEDIA_EXPORT QCameraImageProcessing : public QObject
{
    Q_OBJECT
    Q_PROPERTY(WhiteBalanceMode whiteBalanceMode READ whiteBalanceMode WRITE setWhiteBalanceMode NOTIFY whiteBalanceModeChanged)
    Q_PROPERTY(qreal manualWhiteBalance READ manualWhiteBalance WRITE setManualWhiteBalance NOTIFY manualWhiteBalanceChanged)
    Q_PROPERTY(ColorFilter colorFilter READ colorFilter WRITE setColorFilter NOTIFY colorFilterChanged)
    Q_PROPERTY(qreal brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(qreal contrast READ contrast WRITE setContrast NOTIFY contrastChanged)
    Q_PROPERTY(qreal hue READ hue WRITE setHue NOTIFY hueChanged)
    Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
    Q_ENUMS(WhiteBalanceMode)
    Q_ENUMS(ColorFilter)
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
        WhiteBalanceSunset = 8
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
        ColorFilterAqua
    };

    bool isAvailable() const;

    WhiteBalanceMode whiteBalanceMode() const;
    void setWhiteBalanceMode(WhiteBalanceMode mode);
    Q_INVOKABLE bool isWhiteBalanceModeSupported(WhiteBalanceMode mode) const;

    qreal manualWhiteBalance() const;
    void setManualWhiteBalance(qreal colorTemperature);

    qreal brightness() const;
    void setBrightness(qreal value);

    qreal contrast() const;
    void setContrast(qreal value);

    qreal saturation() const;
    void setSaturation(qreal value);

    qreal hue() const;
    void setHue(qreal value);

    ColorFilter colorFilter() const;
    void setColorFilter(ColorFilter filter);
    Q_INVOKABLE bool isColorFilterSupported(ColorFilter filter) const;

Q_SIGNALS:
    void whiteBalanceModeChanged() const;
    void manualWhiteBalanceChanged() const;

    void brightnessChanged();
    void contrastChanged();
    void saturationChanged();
    void hueChanged();

    void colorFilterChanged();

protected:
    ~QCameraImageProcessing();

private:
    friend class QCamera;
    friend class QCameraPrivate;
    QCameraImageProcessing(QCamera *camera, QPlatformCamera *cameraControl);

    Q_DISABLE_COPY(QCameraImageProcessing)
    Q_DECLARE_PRIVATE(QCameraImageProcessing)
};

QT_END_NAMESPACE

Q_MEDIA_ENUM_DEBUG(QCameraImageProcessing, WhiteBalanceMode)
Q_MEDIA_ENUM_DEBUG(QCameraImageProcessing, ColorFilter)

#endif  // QCAMERAIMAGEPROCESSING_H
